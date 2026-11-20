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

namespace gg {

enum Type : int { DWORD = 0, QWORD = 1, FLOAT = 2, BYTE = 3 };            // BYTE with ':' prefix = text
enum Region : unsigned { ALLOC = 1, DATA = 2, CODE_APP = 4, ANY_RW = 8 }; // ANY_RW is for host testing only

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
    int64_t lo = 0, hi = 0;   // integers / range
    float flo = 0, fhi = 0;   // floats
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
        n.flo = (float)strtod(a.c_str(), nullptr);
        n.fhi = (float)strtod(b.c_str(), nullptr);
    } else {
        n.lo = strtoll(a.c_str(), nullptr, 10);
        n.hi = strtoll(b.c_str(), nullptr, 10);
    }
    return n;
}

inline bool matchAt(const Needle &n, const uint8_t *p) {
    switch (n.type) {
    case DWORD: {
        int32_t v; memcpy(&v, p, 4);
        return n.range ? (v >= n.lo && v <= n.hi) : v == (int32_t)n.lo;
    }
    case QWORD: {
        int64_t v; memcpy(&v, p, 8);
        return n.range ? (v >= n.lo && v <= n.hi) : v == n.lo;
    }
    case FLOAT: {
        float v; memcpy(&v, p, 4);
        if (std::isnan(v)) return false;
        return n.range ? (v >= n.flo && v <= n.fhi) : v == n.flo;
    }
    default: return memcmp(p, n.text.data(), n.text.size()) == 0;
    }
}

// ---------------------------------------------------------------- engine
class Engine {
public:
    std::atomic<bool> busy{false}, cancel{false};
    std::atomic<float> progress{0.f};

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
        if (fd_ < 0) { say("Cannot open /proc/self/mem (SELinux?)"); return; }
        if (f.flags == 1 || f.flags == 3) { say(std::string(f.name) + ": " + f.note); return; }
        if (f.flags == 2) { say(std::string(f.name) + ": original script does nothing here"); return; }
        size_t total = 0, wrote = 0;
        std::vector<Undo> undo;
        for (size_t i = 0; i < f.ops.size() && !cancel; i++) {
            progress = (float)i / (float)f.ops.size();
            size_t n = runOp(f.ops[i], undo, &wrote);
            total += n;
            if (n == 0 && f.flags == 4) { say(std::string(f.name) + ": no value found (not in a match?)"); break; }
        }
        {
            std::lock_guard<std::mutex> g(mu_);
            auto &slot = slotFor(f.id);
            slot.insert(slot.end(), undo.begin(), undo.end());
        }
        progress = 1.f;
        char b[160];
        snprintf(b, sizeof b, "%s: found %zu, edited %zu", f.name, total, wrote);
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
            if (pwrite(fd_, it->old.data(), it->old.size(), (off_t)it->addr) == (ssize_t)it->old.size()) n++;
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
        if (mask & ALLOC)
            if (m.w && (has("libc_malloc") || has("scudo") || has("[heap]") || has("jemalloc"))) return true;
        if (mask & DATA)
            if (m.w && (has(".so") || has(".bss"))) return true;
        if (mask & CODE_APP)
            if (!m.w && has("/data/app/") && (has(".so") || has(".apk") || has(".odex") || has(".oat"))) return true;
        return false;
    }

private:
    int fd_ = -1;
    std::mutex mu_;
    std::vector<uint8_t> scanbuf_;
    std::vector<std::pair<uintptr_t, size_t>> self_; // our own heap objects (sorted): never treat as hits

    void addSelf(const void *p, size_t n) { if (p && n) self_.push_back({(uintptr_t)p, n}); }
    void addSelfStr(const std::string &s) { addSelf(s.data(), s.capacity() + 1); }
    bool isSelf(uintptr_t a, size_t n) const {
        auto it = std::upper_bound(self_.begin(), self_.end(), std::make_pair(a + n, (size_t)0),
                                   [](const std::pair<uintptr_t, size_t> &x, const std::pair<uintptr_t, size_t> &y) { return x.first < y.first; });
        while (it != self_.begin()) {
            --it;
            if (it->first + it->second <= a) { if (it->first + 65536 < a) break; continue; }
            return true;
        }
        return false;
    }
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

    size_t runOp(const Op &op, std::vector<Undo> &undo, size_t *writtenOut = nullptr) {
        std::vector<std::string> sv = split(op.search, ';');
        std::vector<Needle> needles;
        for (auto &s : sv) needles.push_back(makeNeedle(op.type, s));
        std::vector<std::string> ev = split(op.edit, ';');
        {   // register everything we own that contains the values being searched
            self_.clear();
            const size_t CH = 1 << 20;
            size_t maxLen = 1;
            for (auto &n : needles) maxLen = std::max(maxLen, n.size());
            scanbuf_.assign(CH + maxLen, 0);
            addSelf(scanbuf_.data(), scanbuf_.size());
            addSelf(sv.data(), sv.capacity() * sizeof(std::string));
            addSelf(ev.data(), ev.capacity() * sizeof(std::string));
            addSelf(needles.data(), needles.capacity() * sizeof(Needle));
            for (auto &x : sv) addSelfStr(x);
            for (auto &x : ev) addSelfStr(x);
            for (auto &n : needles) addSelfStr(n.text);
            for (auto &u : undo) addSelf(u.old.data(), u.old.capacity());
            std::lock_guard<std::mutex> g(mu_);
            for (auto &slot : undo_) for (auto &u : slot.second) addSelf(u.old.data(), u.old.capacity());
            std::sort(self_.begin(), self_.end());
        }
        bool group = needles.size() > 1;
        size_t limit = op.maxResults > 0 ? (size_t)op.maxResults : 0;

        std::vector<Hit> hits;
        auto maps = readMaps();
        size_t done = 0;
        for (auto &m : maps) {
            if (cancel) break;
            done++;
            if (!inRegion(m, op.region)) continue;
            scanRegion(m, needles, hits, group ? (size_t)2000000 : limit);
            if (!group && limit && hits.size() >= limit) break;
        }
        if (group) hits = groupFilter(hits, needles.size());
        if (limit && hits.size() > limit) hits.resize(limit);

        size_t written = 0;
        for (auto &h : hits) {
            if (ev.empty() || ev[0].empty()) break; // search-only
            const std::string &val = ev.size() == needles.size() ? ev[h.idx] : ev[0];
            if (writeValue(op.editType, val, h.addr, undo)) written++;
        }
        scanbuf_.clear(); scanbuf_.shrink_to_fit();
        if (writtenOut) *writtenOut += written;
        return hits.size();
    }

    void scanRegion(const MapEntry &m, const std::vector<Needle> &needles, std::vector<Hit> &hits, size_t cap) {
        const size_t CH = 1 << 20;
        size_t maxLen = 0, align = 1;
        for (auto &n : needles) maxLen = std::max(maxLen, n.size());
        if (needles[0].type != BYTE) align = needles[0].size();
        std::vector<uint8_t> &buf = scanbuf_;
        for (uintptr_t a = m.start; a < m.end && !cancel; a += CH) {
            size_t want = std::min<uintptr_t>(CH + maxLen - 1, m.end - a);
            ssize_t got = pread(fd_, buf.data(), want, (off_t)a);
            if (got < (ssize_t)maxLen) continue;
            size_t last = (size_t)got - maxLen;
            size_t lim = std::min<size_t>(last, CH - 1); // positions owned by this chunk
            for (size_t o = 0; o <= lim; o++) {
                if (align > 1 && ((a + o) % align)) { o += (align - ((a + o) % align)) - 1; continue; }
                for (size_t k = 0; k < needles.size(); k++) {
                    const Needle &n = needles[k];
                    if (o + n.size() <= (size_t)got && matchAt(n, buf.data() + o)) {
                        if (isSelf(a + o, n.size())) break; // our own copy, not game memory
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
        if (pread(fd_, u.old.data(), len, (off_t)addr) != (ssize_t)len) return false;
        if (pwrite(fd_, b, len, (off_t)addr) != (ssize_t)len) return false;
        undo.push_back(std::move(u));
        return true;
    }
};

} // namespace gg
