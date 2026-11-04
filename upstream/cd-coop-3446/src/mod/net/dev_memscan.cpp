#include "mod/net/dev_memscan.h"

#include <windows.h>

#include <cstring>
#include <string>

#include "mod/game/safe_memory.h"
#include "mod/log.h"
#include "mod/net/dev_watch.h"

namespace cdmp::net {
namespace {

constexpr int kWindow = 64;        // bytes kept on each side of a match
constexpr size_t kMaxMatches = 4000;
constexpr size_t kChunk = 1 << 20;

struct Match {
    uintptr_t addr;
    uint8_t before[kWindow * 2];
};
std::vector<Match> g_matches;

}  // namespace

void dev_memscan(const std::vector<uint8_t>& pattern) {
    g_matches.clear();
    if (pattern.empty() || pattern.size() > kWindow) return;
    std::vector<uint8_t> buf(kChunk + kWindow * 2);
    uint64_t scanned = 0;
    const DWORD t0 = GetTickCount();
    MEMORY_BASIC_INFORMATION mbi{};
    for (uintptr_t a = 0x10000; a < 0x7FFFFFFF0000ull && g_matches.size() < kMaxMatches;
         a = reinterpret_cast<uintptr_t>(mbi.BaseAddress) + mbi.RegionSize) {
        if (!VirtualQuery(reinterpret_cast<void*>(a), &mbi, sizeof mbi)) break;
        if (mbi.State != MEM_COMMIT || mbi.Type != MEM_PRIVATE || mbi.Protect != PAGE_READWRITE) continue;
        const uintptr_t base = reinterpret_cast<uintptr_t>(mbi.BaseAddress);
        for (size_t off = 0; off < mbi.RegionSize && g_matches.size() < kMaxMatches; off += kChunk) {
            const size_t n = std::min(kChunk, mbi.RegionSize - off);
            if (!game::safe_read(base + off, buf.data(), n)) break;
            scanned += n;
            for (size_t i = 0; i + pattern.size() <= n; i += 4) {
                if (std::memcmp(buf.data() + i, pattern.data(), pattern.size()) != 0) continue;
                Match m{base + off + i, {}};
                if (!game::safe_read(m.addr - kWindow, m.before, sizeof m.before)) continue;
                g_matches.push_back(m);
                if (g_matches.size() >= kMaxMatches) break;
            }
        }
    }
    log::info("Dev memscan: {} matches in {} MB of read/write memory ({} ms)", g_matches.size(), scanned >> 20,
              GetTickCount() - t0);
}

void dev_memdiff() {
    int changed = 0, lines = 0;
    for (size_t k = 0; k < g_matches.size(); ++k) {
        const Match& m = g_matches[k];
        uint8_t now[kWindow * 2];
        if (!game::safe_read(m.addr - kWindow, now, sizeof now)) {
            if (lines++ < 150) log::info("Dev memdiff: match {} at 0x{:X} is gone", k, m.addr);
            continue;
        }
        if (std::memcmp(now, m.before, sizeof now) == 0) continue;
        ++changed;
        std::string d;
        int n = 0;
        for (int i = 0; i < kWindow * 2; ++i) {
            if (now[i] == m.before[i]) continue;
            if (++n <= 24) {
                char s[40];
                std::snprintf(s, sizeof s, " %+d:%02x>%02x", i - kWindow, m.before[i], now[i]);
                d += s;
            }
        }
        if (lines++ < 150) {
            std::string ctx;
            for (int i = kWindow - 32; i < kWindow + 32; ++i) {
                char s[4];
                std::snprintf(s, sizeof s, "%02x", m.before[i]);
                ctx += s;
            }
            log::info("Dev memdiff: match {} at 0x{:X}: {} bytes changed{} | before -32..+32: {}", k, m.addr, n, d,
                      ctx);
        }
    }
    log::info("Dev memdiff: {} of {} matches changed", changed, g_matches.size());
}

void dev_memwatch(int rel, const std::vector<uint8_t>& value, int len, int hits, int nth) {
    int seen = 0;
    std::vector<uintptr_t> picks;
    for (size_t k = 0; k < g_matches.size() && !value.empty(); ++k) {
        uint8_t b[16] = {};
        if (value.size() > sizeof b || !game::safe_read(g_matches[k].addr + rel, b, value.size()) ||
            std::memcmp(b, value.data(), value.size()) != 0) {
            continue;
        }
        uint8_t ctx[32] = {};
        game::safe_read(g_matches[k].addr, ctx, sizeof ctx);
        std::string h;
        for (uint8_t c : ctx) {
            char s[4];
            std::snprintf(s, sizeof s, "%02x", c);
            h += s;
        }
        log::info("Dev memwatch: candidate #{} = match {} at 0x{:X}: {}{}", seen, k, g_matches[k].addr, h,
                  seen >= nth && picks.size() < 4 ? "  <- watched" : "");
        if (seen++ >= nth && picks.size() < 4) picks.push_back(g_matches[k].addr + rel);
    }
    if (!picks.empty()) {
        dev_watch_start_many(picks, len, hits);
    } else {
        log::info("Dev memwatch: no candidate #{} ({} seen)", nth, seen);
    }
}

}  // namespace cdmp::net
