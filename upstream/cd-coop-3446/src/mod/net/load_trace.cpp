#include "mod/net/load_trace.h"

#include <windows.h>

#include <atomic>
#include <cstdint>
#include <cstring>
#include <format>
#include <string>

#include <safetyhook.hpp>

#include "mod/game/offsets.h"
#include "mod/game/rtti.h"
#include "mod/game/safe_memory.h"
#include "mod/log.h"

namespace cdmp::net {
namespace {
namespace off = game::build_3460;

// The state steps on 3460 (each: cmp word [rcx+0x5E], from / jnz / mov word [rcx+0x5E], to / ret).
struct Step {
    uintptr_t rva;
    uint16_t from, to;
};
constexpr Step kSteps[] = {{0x1618B40, 2, 4}, {0x1618B50, 4, 8}, {0x1618B60, 8, 0x10}};

// Finishes an actor tree: collects the root and its children, then runs the 8 -> 0x10 step
// (vf 0x78) and vf 0x140 on each (exe+0x5B1AA6).
constexpr uintptr_t kFinishTree = 0x5B1920;
constexpr uint8_t kFinishTreeHead[] = {0x48, 0x89, 0x5C, 0x24, 0x10, 0x55, 0x56, 0x57, 0x41, 0x54, 0x41, 0x55};

// In the "prepare" step (exe+0x5B0CD0), right after `call [rax+0x80]` on each component of each
// actor of the tree: al != 0 means "this component is still loading", which keeps the actor
// out of the finish step. rdi = the component entry ([rdi] = component), r14 = tree entry
// ([r14+8] = actor).
constexpr uintptr_t kComponentBusy = 0x5B0EBF;
constexpr uint8_t kComponentBusyHead[] = {0x84, 0xC0, 0x75, 0x43};  // test al,al / jnz

std::atomic<bool> g_enabled{false};
SafetyHookMid g_hooks[3];
SafetyHookMid g_finish_hook;
SafetyHookMid g_busy_hook;
std::atomic<int> g_lines{0};
uintptr_t g_base = 0;

bool head_ok(const Step& s) {
    const uint8_t want[] = {0x66, 0x83, 0x79, 0x5E, static_cast<uint8_t>(s.from), 0x75, 0x06,
                            0x66, 0xC7, 0x41, 0x5E, static_cast<uint8_t>(s.to), 0x00, 0xC3};
    uint8_t got[sizeof want];
    return game::safe_read(g_base + s.rva, got, sizeof got) && std::memcmp(got, want, sizeof want) == 0;
}

// Exe return addresses on the stack (a cheap backtrace), as rvas.
int exe_frames(uintptr_t rsp, uintptr_t* out, int max) {
    int n = 0;
    __try {
        const auto* sp = reinterpret_cast<const uintptr_t*>(rsp);
        for (int i = 0; i < 96 && n < max; ++i) {
            const uintptr_t v = sp[i];
            if (v > g_base + 0x1000 && v < g_base + 0x558E000) out[n++] = v - g_base;
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
    }
    return n;
}

// The copies (0x?010Fxxx) always; our own player and user a few times, for comparison.
std::atomic<int> g_own_lines{0};
bool wanted(uint32_t key) {
    if ((key & 0x0FFFF000) == 0x0010F000) return g_lines.fetch_add(1) < 2000;
    if (key == 0xA0100001 || key == 0x90100000) return g_own_lines.fetch_add(1) < 40;
    return false;
}

template <int I>
void on_step(safetyhook::Context& ctx) {
    uint32_t key = 0;
    uint16_t state = 0;
    if (!game::safe_read(ctx.rcx + 0x60, &key, 4) || !game::safe_read(ctx.rcx + 0x5E, &state, 2)) return;
    if (!wanted(key)) return;
    uintptr_t frames[6];
    const int n = exe_frames(ctx.rsp, frames, 6);
    std::string text = std::format("LoadTrace: {:08X} step {:X}->{:X} (state now {:X}) | ret", key, kSteps[I].from,
                                   kSteps[I].to, state);
    for (int i = 0; i < n; ++i) text += std::format(" {:X}", frames[i]);
    log::write(log::Level::Info, text);
}

void on_finish(safetyhook::Context& ctx) {
    uint32_t key = 0;
    uint16_t state = 0;
    if (!ctx.rcx || !game::safe_read(ctx.rcx + 0x60, &key, 4) || !game::safe_read(ctx.rcx + 0x5E, &state, 2)) return;
    if (!wanted(key)) return;
    uintptr_t frames[6];
    const int n = exe_frames(ctx.rsp, frames, 6);
    std::string text = std::format("LoadTrace: finish tree, root {:08X} (state {:X}) | ret", key, state);
    for (int i = 0; i < n; ++i) text += std::format(" {:X}", frames[i]);
    log::write(log::Level::Info, text);
}

std::atomic<int> g_busy_lines{0};
void on_component_busy(safetyhook::Context& ctx) {
    if ((ctx.rax & 0xFF) == 0) return;
    uintptr_t actor = 0, comp = 0, vt = 0;
    uint32_t key = 0;
    if (!game::safe_read(ctx.r14 + 8, &actor, 8) || !game::safe_read(actor + 0x60, &key, 4)) return;
    if ((key & 0x0FFFF000) != 0x0010F000) return;  // copies only
    if (!game::safe_read(ctx.rdi, &comp, 8) || !game::safe_read(comp, &vt, 8)) return;
    if (g_busy_lines.fetch_add(1) > 200) return;
    log::info("LoadTrace: copy {:08X} waits on component {} (vtable exe+0x{:X}, comp 0x{:X})", key,
              game::rtti_name_of_vtable(vt), vt - g_base, comp);
}

}  // namespace

void load_trace_enable(bool on) { g_enabled = on; }

void load_trace_install() {
    if (!g_enabled) return;
    g_base = reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
    if (g_base != off::kImageBase) return;
    using Fn = void (*)(safetyhook::Context&);
    constexpr Fn kFns[] = {on_step<0>, on_step<1>, on_step<2>};
    for (int i = 0; i < 3; ++i) {
        if (!head_ok(kSteps[i])) {
            log::warn("LoadTrace: step {:X}->{:X} not recognised at exe+0x{:X}", kSteps[i].from, kSteps[i].to,
                      kSteps[i].rva);
            continue;
        }
        g_hooks[i] = safetyhook::create_mid(reinterpret_cast<void*>(g_base + kSteps[i].rva), kFns[i]);
        log::info("LoadTrace: step {:X}->{:X} hook {}", kSteps[i].from, kSteps[i].to, g_hooks[i] ? "installed" : "FAILED");
    }
    uint8_t head[sizeof kFinishTreeHead];
    if (game::safe_read(g_base + kFinishTree, head, sizeof head) && std::memcmp(head, kFinishTreeHead, sizeof head) == 0) {
        g_finish_hook = safetyhook::create_mid(reinterpret_cast<void*>(g_base + kFinishTree), on_finish);
        log::info("LoadTrace: finish-tree hook {}", g_finish_hook ? "installed" : "FAILED");
    }
    uint8_t busy[sizeof kComponentBusyHead];
    if (game::safe_read(g_base + kComponentBusy, busy, sizeof busy) &&
        std::memcmp(busy, kComponentBusyHead, sizeof busy) == 0) {
        g_busy_hook = safetyhook::create_mid(reinterpret_cast<void*>(g_base + kComponentBusy), on_component_busy);
        log::info("LoadTrace: component-busy hook {}", g_busy_hook ? "installed" : "FAILED");
    }
}

}  // namespace cdmp::net
