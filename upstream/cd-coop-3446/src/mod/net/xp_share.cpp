#include "mod/net/xp_share.h"

#include <windows.h>

#include <atomic>
#include <cstring>
#include <deque>
#include <mutex>

#include "mod/log.h"
#include "mod/net/local_capture.h"
#include "mod/net/prologue_hold.h"
#include "mod/net/quest_advance.h"

namespace cdmp::net {
namespace {

constexpr uint16_t kOpAddSubLevelExperienceAck = 2995;  // u32 player | i64 | u8 | u32 sub-level | u64 amount | u16
constexpr uint64_t kEchoWindowMs = 10000;
constexpr size_t kMaxQueued = 64;

std::atomic<bool> g_on{true};
std::mutex g_m;
// under g_m:
struct Gain {
    uint32_t key;
    uint64_t amount;
    uint64_t at;
};
std::deque<Gain> g_expected;  // our grants still to show up as AddSubLevelExperienceAck (not to send back)
std::deque<Gain> g_incoming;  // the partner's gains to grant here
std::vector<std::vector<uint8_t>> g_out;
uint64_t g_sent = 0, g_granted = 0, g_dropped = 0;

bool skill_points(uint32_t key) { return key >= 521 && key <= 523; }

}  // namespace

void xp_share_configure(bool on) {
    g_on = on;
    if (on) server_trigger_hook_install();
    log::info("Co-op: shared XP {}", on ? "on (every skill-point gain of one player is granted to the other)" : "off");
}

bool xp_share_enabled() { return g_on.load(); }

void xp_share_observe_s2c(const uint8_t* packet, size_t size) {
    if (!g_on || size < 5 + 25) return;
    uint16_t op = 0;
    std::memcpy(&op, packet, 2);
    if (op != kOpAddSubLevelExperienceAck) return;
    const uint8_t* p = packet + 5;
    uint32_t player = 0, key = 0;
    uint64_t amount = 0;
    std::memcpy(&player, p, 4);
    std::memcpy(&key, p + 13, 4);
    std::memcpy(&amount, p + 17, 8);
    if (player != capture_own_player_key() || !skill_points(key) || amount == 0 || amount > 100000) return;
    const uint64_t now = GetTickCount64();
    std::lock_guard lock(g_m);
    while (!g_expected.empty() && now - g_expected.front().at > kEchoWindowMs) g_expected.pop_front();
    for (auto it = g_expected.begin(); it != g_expected.end(); ++it) {
        if (it->key == key && it->amount == amount) {  // our own grant of the partner's gain: not sent back
            g_expected.erase(it);
            return;
        }
    }
    if (prologue_held()) return;  // apart for the prologue: one's own
    std::vector<uint8_t> m{kMsgSharedXp};
    for (int i = 0; i < 4; ++i) m.push_back(static_cast<uint8_t>(key >> (8 * i)));
    for (int i = 0; i < 8; ++i) m.push_back(static_cast<uint8_t>(amount >> (8 * i)));
    if (g_out.size() < kMaxQueued) g_out.push_back(std::move(m));
    if (++g_sent <= 20 || g_sent % 50 == 0) {
        log::info("XP: we gained {} (sub-level {}); shared with the partner ({} so far)", amount, key, g_sent);
    }
}

std::vector<std::vector<uint8_t>> xp_share_take_outgoing() {
    std::lock_guard lock(g_m);
    std::vector<std::vector<uint8_t>> out;
    out.swap(g_out);
    return out;
}

void xp_share_on_message(const std::vector<uint8_t>& bytes) {
    if (bytes.size() < 13 || bytes[0] != kMsgSharedXp) return;
    uint32_t key = 0;
    uint64_t amount = 0;
    std::memcpy(&key, bytes.data() + 1, 4);
    std::memcpy(&amount, bytes.data() + 5, 8);
    if (!skill_points(key) || amount == 0 || amount > 100000) return;
    std::lock_guard lock(g_m);
    if (g_incoming.size() < kMaxQueued) g_incoming.push_back(Gain{key, amount, GetTickCount64()});
}

void xp_share_tick(bool linked) {
    std::deque<Gain> todo;
    {
        std::lock_guard lock(g_m);
        if (g_incoming.empty()) return;
        todo.swap(g_incoming);
    }
    for (const Gain& g : todo) {
        if (!g_on || !linked || prologue_held()) {
            ++g_dropped;
            continue;
        }
        {
            std::lock_guard lock(g_m);
            g_expected.push_back(Gain{g.key, g.amount, GetTickCount64()});
        }
        const bool ok = quest_advance_grant_xp(g.key, g.amount);
        if (!ok) {
            std::lock_guard lock(g_m);
            if (!g_expected.empty()) g_expected.pop_back();
        }
        if (++g_granted <= 20 || g_granted % 50 == 0) {
            log::info("XP: the partner gained {} (sub-level {}); granted here{}", g.amount, g.key, ok ? "" : " - NOT queued");
        }
    }
}

}  // namespace cdmp::net
