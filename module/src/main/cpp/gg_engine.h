// gg_engine.h - tiny in-process port of the GameGuardian calls used by the Respawnables script
// (searchNumber / getResults / editAll) with an undo log. No ImGui dependency.
#pragma once
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <mutex>
#include <string>
#include <thread>
#include <unistd.h>
#include <vector>
#include <algorithm>
#include <cerrno>
#include <csetjmp>
#include <csignal>
#include <sys/mman.h>

// IMPORTANT: on armeabi-v7a/x86 off_t is 32-bit, so addresses above 0x7FFFFFFF turned negative and
// every pread() failed ("found 0"). Always use the 64-bit variants with a zero-extended address.
#define GG_PREAD(fd, buf, n, addr)  pread64((fd), (buf), (n), (off64_t)(uint64_t)(uintptr_t)(addr))
#define GG_PWRITE(fd, buf, n, addr) pwrite64((fd), (buf), (n), (off64_t)(uint64_t)(uintptr_t)(addr))

// ---------------------------------------------------------------- direct memory access with fault guard
// /proc/self/mem is refused on some devices (errno 13), so we also read/write our own address space
// directly. A SIGSEGV/SIGBUS inside the guarded copy is turned into a normal "failed" result.
namespace detail {
inline thread_local sigjmp_buf *tl_jb = nullptr;
inline struct sigaction g_oldSegv, g_oldBus;
inline void guardHandler(int sig, siginfo_t *si, void *ctx) {
    if (tl_jb) siglongjmp(*tl_jb, 1);
    struct sigaction *old = sig == SIGSEGV ? &g_oldSegv : &g_oldBus;
    if (old->sa_flags & SA_SIGINFO) { if (old->sa_sigaction) old->sa_sigaction(sig, si, ctx); return; }
    if (old->sa_handler == SIG_DFL || old->sa_handler == SIG_IGN) { signal(sig, SIG_DFL); return; }
    old->sa_handler(sig);
}
inline void ensureGuard() { // re-installs if the game replaced our handler; chains to whatever was there
    for (int sig : {SIGSEGV, SIGBUS}) {
        struct sigaction cur;
        sigaction(sig, nullptr, &cur);
        if ((cur.sa_flags & SA_SIGINFO) && cur.sa_sigaction == guardHandler) continue;
        struct sigaction sa;
        memset(&sa, 0, sizeof sa);
        sa.sa_sigaction = guardHandler;
        sa.sa_flags = SA_SIGINFO | SA_NODEFER;
        sigemptyset(&sa.sa_mask);
        sigaction(sig, &sa, sig == SIGSEGV ? &g_oldSegv : &g_oldBus);
    }
}
inline bool safeCopy(void *dst, const void *src, size_t n) {
    ensureGuard();
    sigjmp_buf jb;
    tl_jb = &jb;
    volatile bool ok = true;
    if (sigsetjmp(jb, 1) == 0) memcpy(dst, src, n);
    else ok = false;
    tl_jb = nullptr;
    return ok;
}
} // namespace detail

namespace gg {

enum Type : int { DWORD = 0, QWORD = 1, FLOAT = 2, BYTE = 3 };            // BYTE with ':' prefix = text
enum Region : unsigned { ALLOC = 1, DATA = 2, CODE_APP = 4, ANY_RW = 8, ANY_R = 16 }; // ANY_* are for host testing only

struct Op {
    unsigned region;
    int type;
    const char *search;
    int maxResults;
    const char *edit;
    int editType;
};

struct Feature {
    int id;
    const char *name;
    const char *category;
    const char *note;
    int flags; // 0 normal | 1 manual | 2 no-op | 3 not implemented | 4 abort if no result
    std::vector<Op> ops;
};

struct MapEntry {
    uintptr_t start, end;
    bool r, w, x;
    std::string path;
};

struct Undo {
    uintptr_t addr;
    std::vector<uint8_t> old;
};

struct Needle {
    int type = DWORD;
    bool range = false;
    // Stored XOR-masked on purpose: otherwise the scanner finds (and edits!) its own needle in the heap.
    static constexpr uint64_t MASK = 0x5A3C96E1B7D24F08ull;
    uint64_t elo = MASK, ehi = MASK, eflo = MASK, efhi = MASK;
    int64_t lo() const { return (int64_t)(elo ^ MASK); }
    int64_t hi() const { return (int64_t)(ehi ^ MASK); }
    float flo() const { uint32_t u = (uint32_t)(eflo ^ MASK); float f; memcpy(&f, &u, 4); return f; }
    float fhi() const { uint32_t u = (uint32_t)(efhi ^ MASK); float f; memcpy(&f, &u, 4); return f; }
    void setI(int64_t a, int64_t b) { elo = (uint64_t)a ^ MASK; ehi = (uint64_t)b ^ MASK; }
    void setF(float a, float b) {
        uint32_t ua, ub; memcpy(&ua, &a, 4); memcpy(&ub, &b, 4);
        eflo = (uint64_t)ua ^ MASK; efhi = (uint64_t)ub ^ MASK;
    }
    std::string text;         // string search
    size_t size() const {
        if (type == QWORD) return 8;
        if (type == BYTE) return text.size();
        return 4;
    }
};

struct Hit {
    uintptr_t addr;
    int idx; // which search value matched
};

// ---------------------------------------------------------------- helpers
inline std::string stripCommas(std::string s) {
    s.erase(std::remove(s.begin(), s.end(), ','), s.end());
    return s;
}
inline std::vector<std::string> split(const std::string &s, char c) {
    std::vector<std::string> out;
    size_t p = 0;
    for (;;) {
        size_t q = s.find(c, p);
        out.push_back(s.substr(p, q == std::string::npos ? q : q - p));
        if (q == std::string::npos) break;
        p = q + 1;
    }
    return out;
}

inline Needle makeNeedle(int type, const std::string &raw) {
    Needle n;
    n.type = type;
    if (type == BYTE) { // ":text"
        n.text = (!raw.empty() && raw[0] == ':') ? raw.substr(1) : raw;
        return n;
    }
    std::string s = stripCommas(raw);
    size_t tilde = s.find('~');
    std::string a = tilde == std::string::npos ? s : s.substr(0, tilde);
    std::string b = tilde == std::string::npos ? s : s.substr(tilde + 1);
    n.range = tilde != std::string::npos;
    if (type == FLOAT) {
        n.setF((float)strtod(a.c_str(), nullptr), (float)strtod(b.c_str(), nullptr));
    } else {
        n.setI(strtoll(a.c_str(), nullptr, 10), strtoll(b.c_str(), nullptr, 10));
    }
    return n;
}

inline bool matchAt(const Needle &n, const uint8_t *p) {
    switch (n.type) {
    case DWORD: {
        int32_t v; memcpy(&v, p, 4);
        return n.range ? (v >= n.lo() && v <= n.hi()) : v == (int32_t)n.lo();
    }
    case QWORD: {
        int64_t v; memcpy(&v, p, 8);
        return n.range ? (v >= n.lo() && v <= n.hi()) : v == n.lo();
    }
    case FLOAT: {
        float v; memcpy(&v, p, 4);
        if (std::isnan(v)) return false;
        return n.range ? (v >= n.flo() && v <= n.fhi()) : v == n.flo();
    }
    default: return memcmp(p, n.text.data(), n.text.size()) == 0;
    }
}

// ---------------------------------------------------------------- engine
class Engine {
public:
    std::atomic<bool> busy{false}, cancel{false};
    std::atomic<float> progress{0.f};
    size_t statRegions = 0, statBytes = 0, statFails = 0; // last op diagnostics
    int firstErrno = 0;

    Engine() { fd_ = open("/proc/self/mem", O_RDWR | O_CLOEXEC); }
    ~Engine() { if (fd_ >= 0) close(fd_); }

    // run feature on a worker thread
    void runAsync(const Feature &f) {
        if (busy.exchange(true)) return;
        cancel = false;
        std::thread([this, f] { run(f); busy = false; }).detach();
    }
    void revertAsync(int featureId) {
        if (busy.exchange(true)) return;
        std::thread([this, featureId] { revert(featureId); busy = false; }).detach();
    }

    std::vector<std::string> logLines() {
        std::lock_guard<std::mutex> g(mu_);
        return log_;
    }
    bool hasUndo(int id) {
        std::lock_guard<std::mutex> g(mu_);
        for (auto &u : undo_) if (u.first == id && !u.second.empty()) return true;
        return false;
    }

    // ------------------------------------------------ synchronous (also used by tests)
    void run(const Feature &f) {
        if (fd_ < 0) noProc_ = true;
        if (f.flags == 1 || f.flags == 3) { say(std::string(f.name) + ": " + f.note); return; }
        if (f.flags == 2) { say(std::string(f.name) + ": original script does nothing here"); return; }
        firstErrno = 0;
        {
            size_t cA = 0, cD = 0, cC = 0, all = 0;
            for (auto &m : readMaps()) {
                all++;
                if (inRegion(m, ALLOC)) cA++;
                if (inRegion(m, DATA)) cD++;
                if (inRegion(m, CODE_APP)) cC++;
            }
            char d[160];
            snprintf(d, sizeof d, "maps: total=%zu alloc=%zu data=%zu code=%zu ptr=%d-bit", all, cA, cD, cC, (int)sizeof(void *) * 8);
            say(d);
        }
        size_t total = 0, totalEdited = 0, regs = 0, bytes = 0, fails = 0;
        std::vector<Undo> undo;
        for (size_t i = 0; i < f.ops.size() && !cancel; i++) {
            progress = (float)i / (float)f.ops.size();
            size_t edited = 0;
            size_t n = runOp(f.ops[i], undo, edited);
            total += n; totalEdited += edited;
            regs += statRegions; bytes += statBytes; fails += statFails;
            if (n == 0 && f.flags == 4) { say(std::string(f.name) + ": no value found (not in a match?)"); break; }
        }
        {
            std::lock_guard<std::mutex> g(mu_);
            auto &slot = slotFor(f.id);
            slot.insert(slot.end(), undo.begin(), undo.end());
        }
        progress = 1.f;
        char b[240];
        snprintf(b, sizeof b, "%s: found %zu, edited %zu  [%zu regions, %zu MB, %zu read errors, errno=%d, %s]",
                 f.name, total, totalEdited, regs, bytes >> 20, fails, firstErrno, directMode() ? "direct" : "proc");
        say(b);
    }

    void revert(int id) {
        std::vector<Undo> list;
        {
            std::lock_guard<std::mutex> g(mu_);
            auto &slot = slotFor(id);
            list.swap(slot);
        }
        size_t n = 0;
        for (auto it = list.rbegin(); it != list.rend(); ++it)
            if (wrMem(it->addr, it->old.data(), it->old.size())) n++;
        char b[96];
        snprintf(b, sizeof b, "Reverted %zu value(s)", n);
        say(b);
    }

    // Maps classification (GameGuardian-like). Public for tests.
    static std::vector<MapEntry> readMaps() {
        std::vector<MapEntry> out;
        FILE *fp = fopen("/proc/self/maps", "r");
        if (!fp) return out;
        char line[1024];
        while (fgets(line, sizeof line, fp)) {
            unsigned long s, e; char perms[8] = {0}; char path[900] = {0};
            if (sscanf(line, "%lx-%lx %7s %*s %*s %*s %899[^\n]", &s, &e, perms, path) < 3) continue;
            MapEntry m;
            m.start = s; m.end = e;
            m.r = perms[0] == 'r'; m.w = perms[1] == 'w'; m.x = perms[2] == 'x';
            const char *p = path;
            while (*p == ' ') p++;
            m.path = p;
            out.push_back(m);
        }
        fclose(fp);
        return out;
    }
    static bool inRegion(const MapEntry &m, unsigned mask) {
        if (!m.r) return false;
        const std::string &p = m.path;
        auto has = [&](const char *s) { return p.find(s) != std::string::npos; };
        if (mask & ANY_RW) { if (m.w) return true; }
        if (mask & ANY_R) return true;
        if (mask & ALLOC)
            if (m.w && (has("libc_malloc") || has("scudo") || has("[heap]") || has("jemalloc"))) return true;
        if (mask & DATA)
            if (m.w && (has(".so") || has(".bss"))) return true;
        if (mask & CODE_APP)
            if (!m.w && has("/data/app/") && (has(".so") || has(".apk") || has(".odex") || has(".oat"))) return true;
        return false;
    }

private:
    bool noProc_ = false;   // /proc/self/mem refused -> use direct access
public:
    bool forceDirect = false;
    bool directMode() const { return noProc_ || forceDirect; }
private:
    bool rdMem(uintptr_t addr, void *dst, size_t n) {
        if (!directMode()) {
            ssize_t r = GG_PREAD(fd_, dst, n, addr);
            if (r == (ssize_t)n) return true;
            if (r < 0 && (errno == EACCES || errno == EPERM || errno == ENOSYS)) noProc_ = true;
            else return false;
        }
        return detail::safeCopy(dst, (const void *)addr, n);
    }
    static int protOf(uintptr_t addr) {
        for (auto &m : readMaps())
            if (addr >= m.start && addr < m.end)
                return (m.r ? PROT_READ : 0) | (m.w ? PROT_WRITE : 0) | (m.x ? PROT_EXEC : 0);
        return -1;
    }
    bool wrMem(uintptr_t addr, const void *src, size_t n) {
        if (!directMode()) {
            ssize_t r = GG_PWRITE(fd_, src, n, addr);
            if (r == (ssize_t)n) {                         // verify it really landed, else fall back
                uint8_t chk[256];
                if (n <= sizeof chk && detail::safeCopy(chk, (const void *)addr, n) && memcmp(chk, src, n) == 0) return true;
            } else if (r < 0 && (errno == EACCES || errno == EPERM || errno == ENOSYS)) noProc_ = true;
            else if (r < 0) return false;
        }
        bool ok = detail::safeCopy((void *)addr, src, n);   // writable page: done
        if (!ok) {                                           // read-only page: unprotect, write, restore
            int old = protOf(addr);
            if (old < 0) return false;
            uintptr_t pg = addr & ~(uintptr_t)4095, end = (addr + n + 4095) & ~(uintptr_t)4095;
            if (mprotect((void *)pg, end - pg, PROT_READ | PROT_WRITE | (old & PROT_EXEC)) != 0) return false;
            ok = detail::safeCopy((void *)addr, src, n);
            mprotect((void *)pg, end - pg, old);
        }
        __builtin___clear_cache((char *)addr, (char *)addr + n);
        return ok;
    }
    int fd_ = -1;
    std::mutex mu_;
    std::vector<std::string> log_;
    std::vector<std::pair<int, std::vector<Undo>>> undo_;

    std::vector<Undo> &slotFor(int id) {
        for (auto &u : undo_) if (u.first == id) return u.second;
        undo_.push_back({id, {}});
        return undo_.back().second;
    }
    void say(const std::string &s) {
        std::lock_guard<std::mutex> g(mu_);
        log_.push_back(s);
        if (log_.size() > 40) log_.erase(log_.begin());
    }

    size_t runOp(const Op &op, std::vector<Undo> &undo, size_t &written) {
        statRegions = statBytes = statFails = 0;
        std::vector<std::string> sv = split(op.search, ';');
        std::vector<Needle> needles;
        for (auto &s : sv) needles.push_back(makeNeedle(op.type, s));
        bool group = needles.size() > 1;
        size_t limit = op.maxResults > 0 ? (size_t)op.maxResults : 0;

        std::vector<Hit> hits;
        auto maps = readMaps();
        size_t done = 0;
        for (auto &m : maps) {
            if (cancel) break;
            done++;
            if (!inRegion(m, op.region)) continue;
            if (sizeof(void *) == 4 && m.start >= 0xFFFF0000u) continue; // [vectors]
            statRegions++;
            scanRegion(m, needles, hits, group ? (size_t)2000000 : limit);
            if (!group && limit && hits.size() >= limit) break;
        }
        if (group) hits = groupFilter(hits, needles.size());
        if (limit && hits.size() > limit) hits.resize(limit);

        std::vector<std::string> ev = split(op.edit, ';');
        written = 0;
        for (auto &h : hits) {
            if (ev.empty() || ev[0].empty()) break; // search-only
            const std::string &val = ev.size() == needles.size() ? ev[h.idx] : ev[0];
            if (writeValue(op.editType, val, h.addr, undo)) written++;
        }
        return hits.size();
    }

    void scanRegion(const MapEntry &m, const std::vector<Needle> &needles, std::vector<Hit> &hits, size_t cap) {
        const size_t CH = 256 << 10;
        size_t maxLen = 0, align = 1;
        for (auto &n : needles) maxLen = std::max(maxLen, n.size());
        if (needles[0].type != BYTE) align = needles[0].size();
        std::vector<uint8_t> buf(CH + maxLen);
        for (uintptr_t a = m.start; a < m.end && !cancel; a += CH) {
            size_t want = std::min<uintptr_t>(CH + maxLen - 1, m.end - a);
            ssize_t got = rdMem(a, buf.data(), want) ? (ssize_t)want : -1;
            if (got < (ssize_t)maxLen) { if (!statFails) firstErrno = errno; statFails++; continue; }
            statBytes += (size_t)got;
            size_t last = (size_t)got - maxLen;
            size_t lim = std::min<size_t>(last, CH - 1); // positions owned by this chunk
            for (size_t o = 0; o <= lim; o++) {
                if (align > 1 && ((a + o) % align)) { o += (align - ((a + o) % align)) - 1; continue; }
                for (size_t k = 0; k < needles.size(); k++) {
                    const Needle &n = needles[k];
                    if (o + n.size() <= (size_t)got && matchAt(n, buf.data() + o)) {
                        hits.push_back({a + o, (int)k});
                        if (cap && hits.size() >= cap) return;
                        break;
                    }
                }
            }
        }
    }

    // GameGuardian "group search": every value must appear within 512 bytes
    static std::vector<Hit> groupFilter(std::vector<Hit> &hits, size_t k) {
        std::vector<Hit> out;
        std::sort(hits.begin(), hits.end(), [](const Hit &a, const Hit &b) { return a.addr < b.addr; });
        std::vector<bool> keep(hits.size(), false);
        for (size_t i = 0; i < hits.size(); i++) {
            std::vector<bool> seen(k, false);
            size_t cnt = 0, j = i;
            for (; j < hits.size() && hits[j].addr - hits[i].addr < 512; j++)
                if (!seen[hits[j].idx]) { seen[hits[j].idx] = true; cnt++; }
            if (cnt == k) for (size_t t = i; t < j; t++) keep[t] = true;
        }
        for (size_t i = 0; i < hits.size(); i++) if (keep[i]) out.push_back(hits[i]);
        return out;
    }

    bool writeValue(int type, const std::string &val, uintptr_t addr, std::vector<Undo> &undo) {
        uint8_t b[256];
        size_t len = 0;
        if (type == BYTE) {
            if (!val.empty() && val[0] == ':') {
                len = std::min<size_t>(val.size() - 1, sizeof b);
                memcpy(b, val.data() + 1, len);
            } else { b[0] = (uint8_t)strtol(stripCommas(val).c_str(), nullptr, 10); len = 1; }
        } else if (type == FLOAT) {
            float f = (float)strtod(stripCommas(val).c_str(), nullptr);
            memcpy(b, &f, 4); len = 4;
        } else if (type == QWORD) {
            int64_t v = strtoll(stripCommas(val).c_str(), nullptr, 10);
            memcpy(b, &v, 8); len = 8;
        } else {
            int32_t v = (int32_t)strtoll(stripCommas(val).c_str(), nullptr, 10);
            memcpy(b, &v, 4); len = 4;
        }
        Undo u; u.addr = addr; u.old.resize(len);
        if (!rdMem(addr, u.old.data(), len)) return false;
        if (!wrMem(addr, b, len)) return false;
        undo.push_back(std::move(u));
        return true;
    }
};

} // namespace gg
