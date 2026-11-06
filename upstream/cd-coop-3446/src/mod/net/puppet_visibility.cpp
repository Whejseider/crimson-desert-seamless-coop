#include "mod/net/puppet_visibility.h"

#include <windows.h>

#include <atomic>
#include <cstdint>
#include <cstring>
#include <mutex>
#include <unordered_map>

#include <safetyhook.hpp>

#include "mod/game/actor_lookup.h"
#include "mod/game/offsets.h"
#include "mod/game/safe_memory.h"
#include "mod/log.h"
#include "mod/net/enemy_relay.h"

namespace cdmp::net {
namespace {
namespace off = game::build_3576;

constexpr uint32_t kPuppetPlayer = 0xA010F001;
constexpr uint32_t kPuppetHorse = 0xB010F002;
constexpr uint16_t kLoaded = 0x10;

using FlagsChangedFn = void(__fastcall*)(uintptr_t status, uint64_t now, uint64_t before, float fade_s);

uintptr_t g_base = 0;
SafetyHookMid g_hook;
std::atomic<uint64_t> g_shown{0};
// Copies being watched, by actor object (a reloaded copy is a new object and is watched again):
// frames seen since it loaded. A copy is watched for kWatchFrames after loading, in case the
// bit arrives a little after the load, then left alone.
constexpr uint32_t kWatchFrames = 600;  // ~10 s
struct Watch {
    uint32_t key;
    uint32_t frames;
};
std::mutex g_mutex;
std::unordered_map<uintptr_t, Watch> g_watch;

// Co-op+ (F9): one actor of our world drawn hidden (the NPC opponent at a wrestling table while the other player's
// copy sits in its place): the hide bits the game itself uses, set through its own flag-change handler, cleared again
// when released. Bit 20 alone (a loading copy's) did not hide the seated NPC; with bit 2 (the "alpha 0" of hidden
// party members, enemy_relay.cpp) it is gone (live, 2026-11-03).
constexpr uint64_t kHideBit = 0x100004;
std::atomic<uint32_t> g_hide_key{0};
std::atomic<uintptr_t> g_hidden_actor{0};  // the actor we set the bit on (game thread)
std::atomic<uint64_t> g_hide_logs{0};

bool is_copy(uint32_t key) { return key == kPuppetPlayer || key == kPuppetHorse || is_enemy_copy_key(key); }

uintptr_t component(uintptr_t actor, uintptr_t vtable_rva) {
    uintptr_t list = 0;
    uint32_t n = 0;
    if (!game::safe_read(actor + 0x78, &list, 8) || !game::safe_read(actor + 0x80, &n, 4) || n > 128) return 0;
    for (uint32_t i = 0; i < n; ++i) {
        uintptr_t c = 0, vt = 0;
        if (game::safe_read(list + i * 16, &c, 8) && c && game::safe_read(c, &vt, 8) && vt == g_base + vtable_rva) {
            return c;
        }
    }
    return 0;
}

// Clears the bits and lets the game's handler recompute and apply the alpha.
bool show(uintptr_t status, uint64_t flags[2]) {
    const uint64_t before = flags[0] | flags[1];
    const uint64_t a = flags[0] & ~off::kHideFlagsOfCopies, b = flags[1] & ~off::kHideFlagsOfCopies;
    __try {
        *reinterpret_cast<volatile uint64_t*>(status + off::kStatus_HideFlags) = a;
        *reinterpret_cast<volatile uint64_t*>(status + off::kStatus_HideFlags + 8) = b;
        reinterpret_cast<FlagsChangedFn>(g_base + off::kStatusFlagsChanged)(status, a | b, before, 0.0f);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

// Sets (hide) or clears the bit through the game's own flag-change handler, as show() does.
bool set_hidden(uintptr_t actor, bool hide) {
    const uintptr_t status = component(actor, off::kVtStatusComponent);
    uint64_t flags[2] = {};
    if (!status || !game::safe_read(status + off::kStatus_HideFlags, flags, sizeof flags)) return false;
    const uint64_t before = flags[0] | flags[1];
    const uint64_t a = hide ? flags[0] | kHideBit : flags[0] & ~kHideBit;
    if (a == flags[0]) return true;
    __try {
        *reinterpret_cast<volatile uint64_t*>(status + off::kStatus_HideFlags) = a;
        reinterpret_cast<FlagsChangedFn>(g_base + off::kStatusFlagsChanged)(status, a | flags[1], before, 0.0f);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

// The hidden actor's own step: hide it, or show it again once released.
void hide_step(uintptr_t actor, uint32_t key) {
    const uint32_t want = g_hide_key.load();
    if (want && key == want) {
        if (g_hidden_actor.load() == actor) return;
        const bool ok = set_hidden(actor, true);
        if (ok) g_hidden_actor = actor;
        if (g_hide_logs.fetch_add(1) < 20) log::info("Puppet: 0x{:08X} hidden{}", key, ok ? "" : " -- FAILED");
    } else if (g_hidden_actor.load() == actor) {
        const bool ok = set_hidden(actor, false);
        g_hidden_actor = 0;
        if (g_hide_logs.fetch_add(1) < 20) log::info("Puppet: 0x{:08X} shown again{}", key, ok ? "" : " -- FAILED");
    }
}

void on_step(safetyhook::Context& ctx) {
    uintptr_t actor = 0;
    uint32_t key = 0;
    if (!game::safe_read(ctx.rcx + 0x20, &actor, 8) || !actor || !game::safe_read(actor + 0x60, &key, 4)) return;
    if (g_hide_key.load() || g_hidden_actor.load()) hide_step(actor, key);
    if (!is_copy(key)) return;
    uint16_t state = 0;
    if (!game::safe_read(actor + 0x5E, &state, 2) || state != kLoaded) return;  // not loaded yet: next frame
    {
        std::lock_guard lock(g_mutex);
        if (g_watch.size() > 4096) g_watch.clear();
        auto [it, fresh] = g_watch.try_emplace(actor, Watch{key, 0});
        if (!fresh && it->second.key != key) it->second = {key, 0};  // the object was reused
        if (it->second.frames > kWatchFrames) return;
        ++it->second.frames;
    }
    const uintptr_t status = component(actor, off::kVtStatusComponent);
    uint64_t flags[2] = {};
    if (!status || !game::safe_read(status + off::kStatus_HideFlags, flags, sizeof flags)) return;
    if (!((flags[0] | flags[1]) & off::kHideFlagsOfCopies)) return;  // nothing hides it (yet)
    const bool ok = show(status, flags);
    if (g_shown.fetch_add(1) < 50) {
        log::info("Puppet: copy 0x{:08X} made visible (hide flags {:X} cleared){}", key, flags[0] | flags[1],
                  ok ? "" : " -- FAULTED");
    }
}

bool head_ok(uintptr_t rva, const uint8_t* head, size_t n) {
    uint8_t got[16];
    return n <= sizeof got && game::safe_read(g_base + rva, got, n) && std::memcmp(got, head, n) == 0;
}

}  // namespace

void puppet_hide_tick() {
    const uint32_t key = g_hide_key.load();
    if (!key) return;
    const auto actor = game::find_actor(key);
    if (!actor) return;
    const uintptr_t status = component(*actor, off::kVtStatusComponent);
    uint64_t flags[2] = {};
    if (!status || !game::safe_read(status + off::kStatus_HideFlags, flags, sizeof flags)) return;
    // The game clears bit 20 again within ~2 s (live, 2026-11-03), and the actor may not have existed yet when
    // puppet_hide_actor ran: set the bits whenever one is missing.
    if (((flags[0] | flags[1]) & kHideBit) != kHideBit && set_hidden(*actor, true)) g_hidden_actor = *actor;
}

uint32_t puppet_hidden_key() { return g_hidden_actor.load() ? g_hide_key.load() : 0; }

void puppet_hide_actor(uint32_t key) {
    const uint32_t before = g_hide_key.exchange(key);
    if (before == key) return;
    log::info("Puppet: {} 0x{:08X}", key ? "hiding" : "no longer hiding", key ? key : before);
    // A seated minigame NPC runs no movement step (2026-11-03): applied from here as well, through the same handler.
    if (key) {
        if (const auto actor = game::find_actor(key); actor && g_hidden_actor.load() != *actor) {
            const bool ok = set_hidden(*actor, true);
            if (ok) g_hidden_actor = *actor;
            log::info("Puppet: 0x{:08X} hidden{}", key, ok ? "" : " -- FAILED");
        }
    } else if (const uintptr_t actor = g_hidden_actor.exchange(0)) {
        // Only while it still exists (the game unspawns the opponent at the end of the match).
        const auto now = game::find_actor(before);
        const bool ok = now && *now == actor && set_hidden(actor, false);
        log::info("Puppet: 0x{:08X} {}", before, ok ? "shown again" : "gone already");
    }
}

bool puppet_visibility_install() {
    if (g_hook) return true;
    g_base = reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
    uintptr_t slot2 = 0;
    if (g_base != off::kImageBase || !game::safe_read(g_base + off::kVtNormalMovementMethod + 2 * 8, &slot2, 8) ||
        slot2 != g_base + off::kMovementMethodStep ||
        !head_ok(off::kMovementMethodStep, off::kMovementMethodStepHead, sizeof off::kMovementMethodStepHead) ||
        !head_ok(off::kStatusFlagsChanged, off::kStatusFlagsChangedHead, sizeof off::kStatusFlagsChangedHead)) {
        log::warn("Puppet: visibility fix not recognised on this build; copies may stay invisible");
        return false;
    }
    g_hook = safetyhook::create_mid(reinterpret_cast<void*>(slot2), on_step);
    log::info("Puppet: visibility fix {}", g_hook ? "installed" : "FAILED to install");
    return static_cast<bool>(g_hook);
}

}  // namespace cdmp::net
