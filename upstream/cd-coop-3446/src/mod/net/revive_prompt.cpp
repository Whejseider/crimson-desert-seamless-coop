#include "mod/net/revive_prompt.h"

#include <windows.h>

#include <atomic>
#include <cstring>
#include <vector>

#include "mod/game/actor_lookup.h"
#include "mod/game/offsets.h"
#include "mod/game/safe_memory.h"
#include "mod/game/ui_state.h"
#include "mod/net/boss_safe.h"
#include "mod/log.h"
#include "mod/net/local_capture.h"
#include "mod/net/remote_puppet.h"
#include "mod/net/troc_sniffer.h"

namespace cdmp::net {
namespace {
namespace off = game::build_3479;

constexpr uint16_t kOpReviveReq = 2957;             // u32 player | u8 type | u8 x3
constexpr uint16_t kOpReviveLoadingStartReq = 3139;  // 04 | u8 type | 00
constexpr uint16_t kOpReviveLoadingReq = 2390;       // u8 type
constexpr uint16_t kOpGameLoadingStartReq = 2294;    // empty: the client starts loading a field
constexpr uint16_t kOpLoadingFieldDataAck = 2399;    // 04 | u32 player | ... | +28 vec3, quat, scale
constexpr uint16_t kOpDoTeleportAck = 3052;          // u32 actor | vec3 | 8 bytes (handler reads 4, 12, 8)
constexpr uint8_t kInstant = 1, kCheckpoint = 2;
constexpr uint32_t kWaitForGameMs = 4000;   // the game's own screen shows ~2 s after the death
constexpr uint32_t kLoadingStepMs = 1000;   // the game's gap between its two checkpoint requests
constexpr uint32_t kRetryAfterMs = 5000;    // still dead this long after a choice: offer it again
constexpr uint32_t kTeleportCheckMs = 1000; // after the revive: still farther than kTeleportNearM -> again
constexpr float kTeleportNearM = 10.0f;
constexpr int kTeleportTries = 3;
constexpr uint64_t kBossFightWindowMs = 30000;  // a boss whose HP changed this recently: the fight is on
constexpr uint32_t kBossReviveRetryMs = 4000;

uint32_t g_dead_since = 0;
uint32_t g_chosen_at = 0;
uint32_t g_loading_req_at = 0;
uint32_t g_revive_req_at = 0;  // checkpoint step 3 (GameLoadingStartReq + ReviveReq), unless the game loads
bool g_q_was = false, g_u_was = false;
// Checkpoint revive: the server moved us (LoadingFieldDataAck) but the client, not loading, stays put.
std::atomic<bool> g_checkpoint_asked{false}, g_teleport_due{false};
float g_checkpoint[3] = {};
uint32_t g_teleport_at = 0;
int g_teleport_tries = 0;
// A9 (boss-fight deaths).
bool g_boss_wait = true;
bool g_boss_hold = false;       // we died during a boss fight and wait for the other player
uint32_t g_boss_revive_at = 0;  // the boss is down: we were sent up; checked again at this time

// The status component keeps the revive type the client itself asked for at +0x471 (16 = none);
// its ReviveLoadingStartAck handler (exe+0x6F33D0) drops an answer of another type. Set as the
// game's screen would before asking. Not enough for its loading screen; kept because the one
// teleport that took had it (live tests 2026-11-02).
constexpr uint32_t kStatus_PendingReviveType = 0x471;

bool write_byte(uintptr_t addr, uint8_t v) {
    __try {
        *reinterpret_cast<volatile uint8_t*>(addr) = v;
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool expect_revive(uint32_t own, uint8_t type) {
    const auto actor = game::find_actor(own);
    const auto table = actor ? game::read_ptr(*actor + off::kActor_ComponentTable) : std::nullopt;
    const auto status = table ? game::read_ptr(*table + off::kComponents_Status) : std::nullopt;
    const auto before = status ? game::read<uint8_t>(*status + kStatus_PendingReviveType) : std::nullopt;
    const bool ok = before && write_byte(*status + kStatus_PendingReviveType, type);
    log::info("Revive: pending revive type {} -> {}{}", before ? static_cast<int>(*before) : -1, type,
              ok ? "" : " (NOT set)");
    return ok;
}

void teleport(uint32_t own) {
    std::vector<uint8_t> b(24, 0);
    std::memcpy(b.data(), &own, 4);
    std::memcpy(b.data() + 4, g_checkpoint, 12);
    puppet_inject_s2c_batch({{kOpDoTeleportAck, b}});
}

bool send(uint16_t op, const std::vector<uint8_t>& body);

// The checkpoint revive (see the header). `move`: also move the client to the checkpoint.
void ask_checkpoint(uint32_t own, uint32_t now_ms, bool move) {
    expect_revive(own, kCheckpoint);
    const bool ok = send(kOpReviveLoadingStartReq, {0x04, kCheckpoint, 0x00});
    g_chosen_at = now_ms ? now_ms : 1;
    g_loading_req_at = (now_ms + kLoadingStepMs) | 1;
    g_checkpoint_asked = move;
    log::info("Revive: checkpoint revive{}, step 1{}", move ? "" : " (staying here)", ok ? "" : " (NOT sent)");
}

bool send(uint16_t op, const std::vector<uint8_t>& body) {
    std::vector<uint8_t> pkt(5 + body.size());
    const uint16_t len = static_cast<uint16_t>(body.size());
    std::memcpy(pkt.data(), &op, 2);
    pkt[2] = 0;
    std::memcpy(pkt.data() + 3, &len, 2);
    if (!body.empty()) std::memcpy(pkt.data() + 5, body.data(), body.size());
    return sniffer_inject_c2s(std::move(pkt));
}

bool pressed(int vk, bool& was, bool focused) {
    const bool down = focused && (GetAsyncKeyState(vk) & 0x8000) != 0;
    const bool edge = down && !was;
    was = down;
    return edge;
}

}  // namespace

void revive_set_boss_wait(bool on) { g_boss_wait = on; }

RevivePrompt revive_prompt_tick(bool connected, float hp, float hp_max, std::optional<float> partner_hp, bool focused,
                                uint32_t now_ms) {
    // The checkpoint revive's second step goes in on time even if we are no longer "dead" by then.
    if (g_loading_req_at && static_cast<int32_t>(now_ms - g_loading_req_at) >= 0) {
        g_loading_req_at = 0;
        const bool ok = send(kOpReviveLoadingReq, {kCheckpoint});
        g_revive_req_at = (now_ms + 400) | 1;
        log::info("Revive: checkpoint revive, step 2{}", ok ? "" : " (NOT sent)");
    }
    if (g_revive_req_at && static_cast<int32_t>(now_ms - g_revive_req_at) >= 0) {
        g_revive_req_at = 0;
        if (sniffer_world_loading()) {
            log::info("Revive: checkpoint revive: the game is loading the checkpoint itself");
            return RevivePrompt::None;
        }
        std::vector<uint8_t> b(8, 0);
        const uint32_t own = capture_own_player_key();
        std::memcpy(b.data(), &own, 4);
        b[4] = kCheckpoint;
        // Queued together: once GameLoadingStartReq is seen, the sniffer holds anything new of ours.
        const bool ok = own && send(kOpGameLoadingStartReq, {}) && send(kOpReviveReq, b);
        log::info("Revive: checkpoint revive, step 3{}", ok ? "" : " (NOT sent)");
    }
    const bool dead = hp_max > 0.0f && hp <= 0.0f;
    if (!dead && g_teleport_due.exchange(false)) {
        g_teleport_at = (now_ms + kTeleportCheckMs) | 1;
        g_teleport_tries = 1;
    }
    if (g_teleport_at && static_cast<int32_t>(now_ms - g_teleport_at) >= 0) {
        g_teleport_at = 0;
        const uint32_t own = capture_own_player_key();
        const auto at = own ? game::actor_world_position(own) : std::nullopt;
        const float dx = at ? at->x - g_checkpoint[0] : 1e9f, dz = at ? at->z - g_checkpoint[2] : 1e9f;
        if (dx * dx + dz * dz <= kTeleportNearM * kTeleportNearM) {
            if (g_teleport_tries) log::info("Revive: at the checkpoint");
        } else if (own && g_teleport_tries++ < kTeleportTries) {
            teleport(own);
            g_teleport_at = (now_ms + kTeleportCheckMs) | 1;
            log::info("Revive: to the checkpoint again ({:.1f}, {:.1f}, {:.1f}) (try {})", g_checkpoint[0],
                      g_checkpoint[1], g_checkpoint[2], g_teleport_tries);
        } else {
            log::warn("Revive: still not at the checkpoint after {} tries; left where we are", kTeleportTries);
        }
    }
    if (!dead) {
        if (g_dead_since && g_chosen_at) log::info("Revive: we are alive again");
        g_dead_since = g_chosen_at = g_boss_revive_at = 0;
        g_boss_hold = false;
        g_q_was = g_u_was = false;
        return RevivePrompt::None;
    }
    if (!g_dead_since) {
        g_dead_since = now_ms ? now_ms : 1;
        // A9: died during a boss fight -> wait for the other player (decided once, at the death).
        g_boss_hold = g_boss_wait && connected && partner_hp && boss_fight_active(kBossFightWindowMs);
        if (g_boss_hold) log::info("Revive: we died in a boss fight: waiting for the other player");
    }
    // Only where the game's screen is known to stay away, and never over it or another menu.
    if (!connected || !puppet_ever_spawned() || game::ui_menu_open()) return RevivePrompt::None;
    if (now_ms - g_dead_since < kWaitForGameMs) return RevivePrompt::None;
    const uint32_t own = capture_own_player_key();
    if (!own) return RevivePrompt::None;

    bool checkpoint_only = false;
    if (g_boss_hold) {
        if (g_boss_revive_at) {
            // The boss is down and we were sent back up; still dead after a while: ask once more.
            if (static_cast<int32_t>(now_ms - g_boss_revive_at) >= 0) {
                g_boss_revive_at = (now_ms + kBossReviveRetryMs) | 1;
                ask_checkpoint(own, now_ms, /*move=*/false);
                log::info("Revive: the boss is down: we get up (again)");
            }
            return RevivePrompt::None;
        }
        if (!partner_hp) {
            g_boss_hold = false;
            log::info("Revive: the other player is gone: revive as usual");
        } else if (boss_killed_within(now_ms - g_dead_since + 2000)) {
            // Up where we fell, full health: the server's checkpoint revive without the move.
            ask_checkpoint(own, now_ms, /*move=*/false);
            g_boss_revive_at = (now_ms + kBossReviveRetryMs) | 1;
            log::info("Revive: the other player beat the boss: we get up");
            return RevivePrompt::None;
        } else if (*partner_hp <= 0.0f) {
            checkpoint_only = true;  // everyone is down: the fight starts again from the checkpoint
        } else if (!boss_fight_active(kBossFightWindowMs)) {
            g_boss_hold = false;
            log::info("Revive: the boss fight is over without a kill: revive as usual");
        } else {
            return RevivePrompt::WaitBoss;
        }
    }
    if (g_chosen_at && now_ms - g_chosen_at < kRetryAfterMs) return RevivePrompt::None;
    if (!checkpoint_only && pressed('Q', g_q_was, focused)) {
        std::vector<uint8_t> b(8, 0);
        std::memcpy(b.data(), &own, 4);
        b[4] = 0;
        b[5] = kInstant;
        const bool ok = send(kOpReviveReq, b);
        g_chosen_at = now_ms ? now_ms : 1;
        log::info("Revive: instant revive asked{}", ok ? "" : " (NOT sent)");
        return RevivePrompt::None;
    }
    if (pressed('U', g_u_was, focused)) {
        ask_checkpoint(own, now_ms, /*move=*/true);
        return RevivePrompt::None;
    }
    return checkpoint_only ? RevivePrompt::CheckpointOnly : RevivePrompt::Choice;
}

void revive_observe_s2c(const uint8_t* packet, size_t size) {
    if (size < 5 + 40 || (packet[0] | packet[1] << 8) != kOpLoadingFieldDataAck || !g_checkpoint_asked) return;
    uint32_t who = 0;
    std::memcpy(&who, packet + 6, 4);
    if (!who || who != capture_own_player_key()) return;
    std::memcpy(g_checkpoint, packet + 5 + 28, 12);
    g_checkpoint_asked = false;
    // At once, with the revive (best effort: revive_prompt.h).
    teleport(who);
    log::info("Revive: to the checkpoint at ({:.1f}, {:.1f}, {:.1f})", g_checkpoint[0], g_checkpoint[1], g_checkpoint[2]);
    g_teleport_due = true;  // checked (and tried again) once we are alive
}

}  // namespace cdmp::net
