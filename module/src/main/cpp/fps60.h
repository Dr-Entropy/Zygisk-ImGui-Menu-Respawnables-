// fps60.h - port of FPS_60_Bounty_Ruch.lua (FPS 60 toggle, offline use)
// Finds the start of libunity.so's .bss, checks two DWORDs at fixed offsets (must be 30 or 60),
// sets them to 60 and keeps them there ("freeze") until disabled, then restores the originals.
#pragma once
#include "gg_engine.h"
#include <chrono>

namespace gg {

class Fps60 {
public:
    explicit Fps60(Engine &e, uintptr_t baseOverride = 0) : e_(e), baseOverride_(baseOverride) {}
    ~Fps60() { stopThread(); }

    bool enabled() const { return enabled_; }

    void enableAsync() {
        if (e_.busy.exchange(true)) return;
        std::thread([this] { enable(); e_.busy = false; }).detach();
    }
    void disableAsync() {
        if (e_.busy.exchange(true)) return;
        std::thread([this] { disable(); e_.busy = false; }).detach();
    }

    // GameGuardian's getRangesList() internalName "libunity.so:bss": first such range wins.
    static uintptr_t pickBase(const std::vector<MapEntry> &maps) {
        for (auto &m : maps)
            if (m.path.find("libunity.so:bss") != std::string::npos) return m.start;
        uintptr_t lastEnd = 0; // fallback: the .bss mapping right after the last libunity.so mapping
        for (auto &m : maps) {
            bool isLib = m.path.find("libunity.so") != std::string::npos;
            if (isLib) { lastEnd = m.end; continue; }
            if (lastEnd && m.start == lastEnd && m.path.find("bss") != std::string::npos) return m.start;
            lastEnd = 0;
        }
        return 0;
    }

    bool enable() {
        std::lock_guard<std::mutex> g(mu_);
        if (enabled_) { e_.log("FPS 60: already enabled"); return true; }
        uintptr_t base = baseOverride_ ? baseOverride_ : pickBase(Engine::readMaps());
        if (!base) { e_.log("FPS 60: libunity.so:bss not found - enter the game fully, then retry"); return false; }
        uintptr_t addr[2];
        int32_t val[2];
        for (int i = 0; i < 2; i++) {
            addr[i] = (uintptr_t)((intptr_t)base + kOffsets[i]);
            if (!e_.rdMem(addr[i], &val[i], 4)) { e_.log("FPS 60: cannot read offset " + std::to_string(i + 1)); return false; }
            if (val[i] != 30 && val[i] != 60) {
                e_.log("FPS 60: offset mismatch, not enabled (value " + std::to_string(val[i]) + ")");
                return false;
            }
        }
        for (int i = 0; i < 2; i++) { addr_[i] = addr[i]; orig_[i] = val[i]; }
        int32_t sixty = 60;
        for (int i = 0; i < 2; i++) e_.wrMem(addr_[i], &sixty, 4);
        enabled_ = true;
        startThread();
        e_.log("FPS 60: enabled");
        return true;
    }

    void disable() {
        stopThread();
        std::lock_guard<std::mutex> g(mu_);
        if (!enabled_) { e_.log("FPS 60: already disabled"); return; }
        for (int i = 0; i < 2; i++) e_.wrMem(addr_[i], &orig_[i], 4);
        enabled_ = false;
        e_.log("FPS 60: disabled (original values restored)");
    }

private:
    static constexpr int64_t kOffsets[2] = {-95724, 437776};
    Engine &e_;
    uintptr_t baseOverride_;
    std::mutex mu_;
    std::atomic<bool> enabled_{false}, run_{false};
    uintptr_t addr_[2] = {0, 0};
    int32_t orig_[2] = {0, 0};
    std::thread th_;

    void startThread() {
        run_ = true;
        th_ = std::thread([this] {              // "freeze": rewrite 60 whenever the game changes it
            while (run_) {
                for (int i = 0; i < 2; i++) {
                    int32_t cur = 0;
                    if (e_.rdMem(addr_[i], &cur, 4) && cur != 60) { int32_t v = 60; e_.wrMem(addr_[i], &v, 4); }
                }
                for (int k = 0; k < 5 && run_; k++) std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        });
    }
    void stopThread() {
        run_ = false;
        if (th_.joinable()) th_.join();
    }
};
constexpr int64_t Fps60::kOffsets[2];

} // namespace gg
