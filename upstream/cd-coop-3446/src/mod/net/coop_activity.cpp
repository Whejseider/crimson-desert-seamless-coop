#include "mod/net/coop_activity.h"

#include <windows.h>

#include <atomic>
#include <cmath>
#include <cstring>
#include <format>
#include <mutex>

#include "mod/game/ui_state.h"
#include "mod/log.h"
#include "mod/net/boss_safe.h"
#include "mod/net/local_capture.h"

namespace cdmp::net {
namespace {

constexpr uint16_t kOpActivateMiniGameReq = 2507;  // u16 minigame id | u32 our player
constexpr uint16_t kOpLeaveMiniGameReq = 2379;     // u8 4 | u32 actor | 14 x 0 | u32 stage key
constexpr float kNearM = 100.0f;                   // the user's rule: cutscenes are shared within 100 m
constexpr uint32_t kSkipTestDelayMs = 3000;
constexpr uint32_t kInjectQuietMs = 800;           // our own injected Esc is not a skip of ours

enum Type : uint8_t { kCutsceneOn = 1, kCutsceneOff = 2, kSkip = 3, kGameOn = 4, kGameOff = 5 };

std::mutex g_mutex;
uint32_t g_seq_out = 1;
uint32_t g_seq_in = 0;               // newest sequence seen from the other player
bool g_partner_cutscene = false;
std::array<float, 3> g_partner_cut_pos{};
std::vector<std::string> g_toasts;   // filled by events, drained by the tick
bool g_skip_wanted = false;          // the other player pressed skip, apply it if we are in a cutscene
std::array<float, 3> g_skip_from{};
bool g_last_cutscene = false;
bool g_esc_was_down = false;
uint32_t g_cut_since = 0;
uint32_t g_injected_at = 0;
bool g_skip_test_done = false;
std::atomic<bool> g_skip_test{false};

void send(Type t, const void* data, size_t n) {
    std::vector<uint8_t> b(5 + n);
    uint32_t seq;
    {
        std::lock_guard lock(g_mutex);
        seq = g_seq_out++;
    }
    std::memcpy(b.data(), &seq, 4);
    b[4] = t;
    if (n) std::memcpy(b.data() + 5, data, n);
    capture_emit_activity(std::move(b));
}

float dist_xz(const std::array<float, 3>& a, const std::array<float, 3>& b) {
    return std::hypot(a[0] - b[0], a[2] - b[2]);
}

void press_escape() {
    const UINT scan = MapVirtualKeyW(VK_ESCAPE, MAPVK_VK_TO_VSC);
    keybd_event(VK_ESCAPE, static_cast<BYTE>(scan), 0, 0);
    Sleep(40);
    keybd_event(VK_ESCAPE, static_cast<BYTE>(scan), KEYEVENTF_KEYUP, 0);
}

}  // namespace

std::string activity_minigame_name(uint32_t stage) {
    switch (stage) {
        case 16005100: case 1001987: case 1001988: case 1003873: case 1003875: return "arm wrestling";
        case 1018693: return "hand wrestling";
        case 16000200: case 1000804: return "the archery contest";
        case 1016639: return "the firearm contest";
        case 1000750: case 1012872: return "an unarmed duel";
        case 1018347: return "a wrestling match";
        case 1000302: return "a spear duel";
        case 1018695: return "a sword and shield duel";
        case 1001120: case 1002460: case 1023356: return "a horse race";
        case 1001949: case 1001963: case 1001964: case 1001969: case 16000600: case 1001970: case 1001974:
            return "rock, paper, scissors";
        case 1000219: case 1001269: return "Duo";
        case 1001524: case 1018696: return "Five-Card";
        default: return "a minigame";
    }
}

void activity_set_skip_test(bool on) { g_skip_test = on; }

void activity_observe_c2s(const uint8_t* packet, size_t size) {
    if (size < 5) return;
    uint16_t op = 0;
    std::memcpy(&op, packet, 2);
    if (op == kOpActivateMiniGameReq && size >= 5 + 2) {
        uint16_t id = 0;
        std::memcpy(&id, packet + 5, 2);
        log::info("Activity: we started a minigame (id 0x{:04X})", id);
        send(kGameOn, &id, 2);
    } else if (op == kOpLeaveMiniGameReq && size >= 5 + 23) {
        uint32_t stage = 0;
        std::memcpy(&stage, packet + 5 + 19, 4);
        log::info("Activity: we finished {} (stage {})", activity_minigame_name(stage), stage);
        send(kGameOff, &stage, 4);
    }
}

void activity_on_event(const std::vector<uint8_t>& p) {
    if (p.size() < 5) return;
    uint32_t seq = 0;
    std::memcpy(&seq, p.data(), 4);
    std::lock_guard lock(g_mutex);
    if (seq <= g_seq_in && g_seq_in - seq < 0x10000) return;  // a repeat (each event is sent three times)
    g_seq_in = seq;
    std::array<float, 3> pos{};
    if (p.size() >= 5 + 12) std::memcpy(pos.data(), p.data() + 5, 12);
    switch (p[4]) {
        case kCutsceneOn: g_partner_cutscene = true; g_partner_cut_pos = pos; break;
        case kCutsceneOff: g_partner_cutscene = false; break;
        case kSkip: g_skip_wanted = true; g_skip_from = pos; break;
        case kGameOn: g_toasts.push_back("{} is playing a minigame"); break;
        case kGameOff: {
            uint32_t stage = 0;
            if (p.size() >= 9) std::memcpy(&stage, p.data() + 5, 4);
            g_toasts.push_back("{} finished " + activity_minigame_name(stage));
            break;
        }
        default: break;
    }
}

ActivityTick activity_tick(bool connected, bool focused, std::optional<std::array<float, 3>> me,
                           std::optional<std::array<float, 3>> partner, const std::string& partner_name,
                           uint32_t now) {
    ActivityTick out;
    const bool cut = game::cutscene_playing().value_or(false);
    boss_note_cutscene(cut);
    const std::array<float, 3> here = me.value_or(std::array<float, 3>{});
    if (cut != g_last_cutscene) {
        g_last_cutscene = cut;
        g_cut_since = now;
        g_skip_test_done = false;
        log::info("Activity: our cutscene {}", cut ? "started" : "ended");
        if (connected) {
            if (cut) send(kCutsceneOn, here.data(), 12);
            else send(kCutsceneOff, nullptr, 0);
        }
    }
    // Our skip: Esc while our cutscene plays (not the Esc we pressed for the other player).
    const bool esc = focused && (GetAsyncKeyState(VK_ESCAPE) & 0x8000) != 0;
    if (esc && !g_esc_was_down && cut && connected && now - g_injected_at > kInjectQuietMs) {
        log::info("Activity: we skipped our cutscene; telling the other player");
        send(kSkip, here.data(), 12);
    }
    g_esc_was_down = esc;
    const std::string who = partner_name.empty() ? std::string("Your friend") : partner_name;
    bool apply_skip = false;
    {
        std::lock_guard lock(g_mutex);
        if (!connected) {
            g_partner_cutscene = false;
            g_skip_wanted = false;
            g_toasts.clear();
        }
        if (g_skip_test && cut && !g_skip_test_done && now - g_cut_since > kSkipTestDelayMs) {
            g_skip_test_done = true;
            g_skip_wanted = true;
            g_skip_from = here;
            log::info("Activity: [dev] skip_test: acting as if the other player pressed skip");
        }
        if (g_skip_wanted) {
            g_skip_wanted = false;
            const bool is_near = !me || dist_xz(here, g_skip_from) <= kNearM;
            apply_skip = cut && is_near && focused;
            if (cut && !apply_skip) {
                log::info("Activity: the other player skipped; ours not skipped ({})", is_near ? "game not in front" : "too far");
            }
        }
        if (!g_toasts.empty()) {
            out.toast = std::vformat(g_toasts.front(), std::make_format_args(who));
            g_toasts.erase(g_toasts.begin());
        }
        const bool is_near = !partner || !me || dist_xz(*partner, here) <= kNearM;
        if (g_partner_cutscene && is_near && !cut) {
            out.banner = std::format("{} is watching a cutscene", who);
        } else if (g_partner_cutscene && cut) {
            out.banner = std::format("{} is in a cutscene too: Esc skips it for both of you", who);
        }
    }
    if (apply_skip) {
        g_injected_at = now;
        press_escape();
        log::info("Activity: the other player skipped; skipping ours too");
    }
    return out;
}

}  // namespace cdmp::net
