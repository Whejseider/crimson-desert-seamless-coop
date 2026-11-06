#include "mod/net/lag_probe.h"

#include <windows.h>

#include <atomic>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "mod/game/actor_lookup.h"
#include "mod/log.h"
#include "mod/net/local_capture.h"
#include "mod/net/remote_puppet.h"

namespace cdmp::net {
namespace {

std::atomic<bool> g_on{false};
std::mutex g_mutex;
std::vector<std::string> g_lines;  // waiting for the writer

int64_t now_us() {
    return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch())
        .count();
}

void push(std::string line) {
    std::lock_guard lock(g_mutex);
    if (g_lines.size() < 200000) g_lines.push_back(std::move(line));
}

void run() {
    wchar_t exe[MAX_PATH] = {};
    GetModuleFileNameW(nullptr, exe, MAX_PATH);
    const auto path = std::filesystem::path(exe).parent_path() / L"cdmp_lag.csv";
    FILE* f = _wfopen(path.c_str(), L"w");
    if (!f) {
        log::warn("Lag probe: cannot write {}", path.string());
        return;
    }
    std::fputs("t_us,kind,a,b,c,d\n", f);
    // 4 ms samples need the 1 ms timer (the network thread sets it too); through winmm.dll, our loader's name.
    if (const HMODULE winmm = GetModuleHandleW(L"winmm.dll")) {
        if (const auto begin = reinterpret_cast<unsigned(__stdcall*)(unsigned)>(GetProcAddress(winmm, "timeBeginPeriod"))) {
            begin(1);
        }
    }
    log::info("Lag probe: on, writing {}", path.string());
    int64_t last_flush = now_us();
    for (;;) {
        const int64_t t = now_us();
        if (puppet_spawned()) {
            const uint32_t own = capture_own_player_key();
            const auto me = own ? game::actor_world_position(own) : std::nullopt;
            const auto copy = puppet_position();
            if (me && copy) {
                char b[160];
                std::snprintf(b, sizeof b, "%lld,0,%.3f,%.3f,%.3f,%.3f\n", static_cast<long long>(t), me->x, me->z,
                              copy->x - puppet_offset_m(), copy->z);
                push(b);
            }
        }
        if (t - last_flush > 1'000'000) {
            last_flush = t;
            std::vector<std::string> out;
            {
                std::lock_guard lock(g_mutex);
                out.swap(g_lines);
            }
            for (const auto& l : out) std::fputs(l.c_str(), f);
            std::fflush(f);
        }
        Sleep(4);
    }
}

}  // namespace

void lag_probe_start() {
    if (g_on.exchange(true)) return;
    std::thread(run).detach();
}

bool lag_probe_on() { return g_on.load(std::memory_order_relaxed); }

void lag_probe_mark(int kind, uint64_t ts) {
    if (!lag_probe_on()) return;
    char b[96];
    std::snprintf(b, sizeof b, "%lld,%d,%llu,0,0,0\n", static_cast<long long>(now_us()), kind,
                  static_cast<unsigned long long>(ts));
    push(b);
}

}  // namespace cdmp::net
