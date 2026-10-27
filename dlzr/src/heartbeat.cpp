// heartbeat.cpp — 卡死诊断实现（见 heartbeat.hpp）
#include "heartbeat.hpp"

#include <cstdio>
#include <cstring>
#include <atomic>
#include <chrono>
#include <thread>

namespace {

FILE* g_hb = nullptr;
std::atomic<uint64_t> g_frame{0};
char g_step[256] = {0};
char g_mode[64] = {0};

// 写 g_step / g_mode 的策略：逐字节拷贝，遇到 '\0' 截断。
// 与看门狗线程存在竞争，但仅用于诊断，无需加锁。
inline void copy_str(char* dst, size_t dst_n, const char* src) {
    size_t i = 0;
    for (; i + 1 < dst_n && src[i] != '\0'; ++i) dst[i] = src[i];
    dst[i] = '\0';
}

}  // namespace

namespace dlzr {

void hb_init() {
    g_hb = std::fopen("dlzr_heartbeat.log", "w");
    if (g_hb) {
        std::fprintf(g_hb, "HB start\n");
        std::fflush(g_hb);
    }
}

void hb_step(const char* s) {
    copy_str(g_step, sizeof(g_step), s);
    if (!g_hb) return;
    std::fprintf(g_hb, "STEP %s frame=%llu\n",
                 s, static_cast<unsigned long long>(g_frame.load()));
    std::fflush(g_hb);
}

void hb_mode(const char* m) {
    copy_str(g_mode, sizeof(g_mode), m);
    if (!g_hb) return;
    std::fprintf(g_hb, "MODE %s frame=%llu\n",
                 m, static_cast<unsigned long long>(g_frame.load()));
    std::fflush(g_hb);
}

void hb_frame(uint64_t f) {
    g_frame.store(f);
    if (g_hb && (f % 30 == 0)) {
        std::fprintf(g_hb, "HB frame=%llu\n", static_cast<unsigned long long>(f));
        std::fflush(g_hb);
    }
}

void hb_watchdog() {
    uint64_t last = 0;
    uint64_t stalled_ms = 0;
    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        uint64_t f = g_frame.load();
        if (f == last) {
            stalled_ms += 1000;
            if (stalled_ms >= 3000 && g_hb) {
                std::fprintf(g_hb,
                    "WATCHDOG STALL: mode=%s step=%s frame=%llu stalled_ms=%llu\n",
                    g_mode, g_step,
                    static_cast<unsigned long long>(f),
                    static_cast<unsigned long long>(stalled_ms));
                std::fflush(g_hb);
            }
        } else {
            last = f;
            stalled_ms = 0;
        }
    }
}

}  // namespace dlzr
