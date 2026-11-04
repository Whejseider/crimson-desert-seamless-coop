#include "mod/net/quest_advance.h"

#include <windows.h>

#include <atomic>
#include <cstring>
#include <deque>
#include <mutex>
#include <vector>

#include <safetyhook.hpp>

#include "mod/game/offsets.h"
#include "mod/game/safe_memory.h"
#include "mod/log.h"
#include "mod/net/troc_sniffer.h"

namespace cdmp::net {
namespace {
namespace off = game::build_3527;

// exe+0x1C3F0F0(manager, u32* mission id, ctx, u8 state, changes*): the server queues a mission state change.
// Called for every mission while a save loads, so the manager is known before anyone needs it.
constexpr uintptr_t kSetMissionState = 0x1C3F0F0;
constexpr uint8_t kSetMissionStateHead[16] = {0x48, 0x89, 0x5C, 0x24, 0x18, 0x44, 0x88, 0x4C,
                                              0x24, 0x20, 0x48, 0x89, 0x54, 0x24, 0x10, 0x55};
// exe+0x2B95BE0: the quest advancement routine (quest_advance.h).
constexpr uintptr_t kAdvance = 0x2B95BE0;
constexpr uint8_t kAdvanceHead[16] = {0x48, 0x89, 0x5C, 0x24, 0x10, 0x55, 0x56, 0x57,
                                      0x41, 0x54, 0x41, 0x55, 0x41, 0x56, 0x41, 0x57};
// MissionTriggerInOutReq's handler (runs on the quest server's thread).
constexpr uintptr_t kTriggerHandler = 0x2EEE800;
constexpr uint8_t kTriggerHandlerHead[16] = {0x48, 0x89, 0x5C, 0x24, 0x08, 0x48, 0x89, 0x74,
                                             0x24, 0x10, 0x48, 0x89, 0x7C, 0x24, 0x18, 0x4C};
// The mission static data table (exe+0x4DBA40 reads it): [+0x08] u32 count, [+0x58] info* per index (null until
// loaded); info +0x08 = the mission key (as in UpdateMissionListAck).
constexpr uintptr_t kMissionTable = 0x71C1940;
constexpr uint64_t kRequestMs = 10000;

using AdvanceFn = void*(__fastcall*)(void* manager, uint32_t* result, uint16_t* index, uint64_t* time, uint8_t flag);

struct Request {
    uint32_t key;
    uint64_t at;
};

uintptr_t g_base = 0;
SafetyHookMid g_state_hook, g_trigger_hook;
std::atomic<uintptr_t> g_manager{0};
std::mutex g_m;
std::deque<Request> g_queue;

bool head_is(uintptr_t rva, const uint8_t* want, size_t n) {
    uint8_t head[16] = {};
    return n <= sizeof head && game::safe_read(g_base + rva, head, n) && std::memcmp(head, want, n) == 0;
}

void on_set_state(safetyhook::Context& ctx) { g_manager.store(ctx.rcx, std::memory_order_relaxed); }

// The index of the loaded mission with this key, or -1.
int mission_index(uint32_t key) {
    uintptr_t table = 0, arr = 0;
    uint32_t count = 0;
    if (!game::safe_read(g_base + kMissionTable, &table, 8) || !game::safe_read(table + 8, &count, 4) ||
        !game::safe_read(table + 0x58, &arr, 8) || count > 0x10000) {
        return -1;
    }
    for (uint32_t i = 0; i < count; ++i) {
        uintptr_t info = 0;
        uint32_t k = 0;
        if (game::safe_read(arr + i * 8ull, &info, 8) && info && game::safe_read(info + 8, &k, 4) && k == key) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

void on_trigger(safetyhook::Context&) {
    std::vector<uint32_t> keys;
    {
        std::lock_guard lock(g_m);
        if (g_queue.empty()) return;
        const uint64_t now = GetTickCount64();
        for (const auto& r : g_queue) {
            if (now - r.at <= kRequestMs) keys.push_back(r.key);
        }
        g_queue.clear();
    }
    const uintptr_t mgr = g_manager.load();
    for (uint32_t key : keys) {
        const int idx = mission_index(key);
        if (!mgr || idx < 0) {
            log::warn("Live quests: step {:08X} not advanced (quest data not found here)", key);
            continue;
        }
        uint32_t result = 0;
        uint16_t index = static_cast<uint16_t>(idx);
        uint64_t when = 0;  // the game's clock time; 0 was accepted for go-to, defeat and talk steps
        reinterpret_cast<AdvanceFn>(g_base + kAdvance)(reinterpret_cast<void*>(mgr), &result, &index, &when, 0);
        log::info("Live quests: the quest server advanced step {:08X} (index {}, result {:X})", key, idx, result);
    }
}

}  // namespace

bool quest_advance_install() {
    if (g_trigger_hook) return true;
    g_base = reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
    if (g_base != off::kImageBase || !head_is(kSetMissionState, kSetMissionStateHead, sizeof kSetMissionStateHead) ||
        !head_is(kAdvance, kAdvanceHead, sizeof kAdvanceHead) ||
        !head_is(kTriggerHandler, kTriggerHandlerHead, sizeof kTriggerHandlerHead)) {
        log::warn("Live quests: the quest server's code is not as expected; steps other than go-to will not follow");
        return false;
    }
    g_state_hook = safetyhook::create_mid(reinterpret_cast<void*>(g_base + kSetMissionState), on_set_state);
    g_trigger_hook = safetyhook::create_mid(reinterpret_cast<void*>(g_base + kTriggerHandler), on_trigger);
    const bool ok = g_state_hook && g_trigger_hook;
    log::info("Live quests: quest server hooks {}", ok ? "installed" : "FAILED");
    return ok;
}

bool quest_advance_ready() { return g_trigger_hook && g_manager.load() != 0; }

void quest_advance_request(uint32_t mission_key) {
    {
        std::lock_guard lock(g_m);
        if (g_queue.size() >= 32) return;
        g_queue.push_back(Request{mission_key, GetTickCount64()});
    }
    // A harmless trigger (mission 0) so the server's handler runs soon.
    std::vector<uint8_t> pkt(5 + 9, 0);
    const uint16_t op = 2899, len = 9;
    std::memcpy(pkt.data(), &op, 2);
    std::memcpy(pkt.data() + 3, &len, 2);
    sniffer_inject_c2s(std::move(pkt));
}

}  // namespace cdmp::net
