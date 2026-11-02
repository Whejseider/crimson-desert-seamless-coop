#include "mod/net/coop_activity.h"

#include <windows.h>

#include <atomic>
#include <cmath>
#include <cstring>
#include <algorithm>
#include <format>
#include <mutex>
#include <thread>

#include "mod/game/ui_state.h"
#include "mod/log.h"
#include "mod/net/boss_safe.h"
#include "mod/net/local_capture.h"

namespace cdmp::net {
namespace {

constexpr uint16_t kOpActivateMiniGameReq = 2507;  // u16 minigame id | u32 our player
constexpr uint16_t kOpLeaveMiniGameReq = 2379;     // u8 4 | u32 actor | 14 x 0 | u32 stage key
constexpr uint16_t kOpSpawnMiniGameCharacterReq = 3124;  // every match: the NPC opponent is spawned
constexpr float kNearM = 100.0f;                   // the user's rule: cutscenes are shared within 100 m
constexpr uint32_t kSkipTestDelayMs = 6000;

enum Type : uint8_t { kCutsceneOn = 1, kCutsceneOff = 2, kSkip = 3, kGameOn = 4, kGameOff = 5, kVersus = 6 };

// Versus arm wrestling (D35): while both players are in a match, the host owns one gauge,
// 50 + kPushGain x (host's E presses - guest's E presses), clamped to 0..100, and writes it into its game every tick;
// the guest writes 100 - that into its own. 100 = a win, 0 = a loss: each game then ends its match by itself.
constexpr float kPushGain = 2.0f;
constexpr uint32_t kVersusSendMs = 100;
std::atomic<uint32_t> g_e_presses{0};     // E key-downs (the hook counts them; not ours)
std::atomic<bool> g_match_on{false};      // between SpawnMiniGameCharacterReq and LeaveMiniGameReq
std::atomic<int> g_versus_test_rate{0};   // [dev] versus_test: a simulated other player pushing this many times/s
struct Versus {
    bool live = false;          // our match is running (gauge readable, no cutscene)
    uint32_t base = 0;          // our presses when the versus started
    bool started = false;
    float gauge = 50.0f;        // the shared gauge, as the host sees it
    uint32_t last_send = 0;
    uint32_t live_since = 0;
    bool partner_live = false;  // from their last kVersus
    uint32_t partner_count = 0; // their presses since their match started
    float partner_gauge = 50.0f;
    uint32_t partner_seen = 0;
    bool announced = false;
    float last_ours = 50.0f;    // our side of the gauge at the last versus tick
} g_vs;

std::mutex g_mutex;
uint32_t g_seq_out = 1;
uint32_t g_seq_in = 0;               // newest sequence seen from the other player
bool g_partner_cutscene = false;
std::array<float, 3> g_partner_cut_pos{};
std::vector<std::string> g_toasts;   // filled by events, drained by the tick
bool g_partner_skip = false;         // the other player pressed skip in their current cutscene
std::array<float, 3> g_skip_from{};
bool g_our_skip = false;             // we pressed skip in our current cutscene
bool g_released = false;             // both pressed: our Esc went to the game
bool g_last_cutscene = false;
uint32_t g_cut_since = 0;
std::atomic<bool> g_hold_esc{false};  // the keyboard hook holds back the player's Esc
std::atomic<uint64_t> g_hold_seen{0};  // GetTickCount64 of the last tick that wanted it (stale = let Esc through)
std::atomic<bool> g_esc_held{false};  // it did (a press to count as our skip)
constexpr ULONG_PTR kOurKey = 0xC0C0A7;  // dwExtraInfo of the Esc we press ourselves (the hook lets it through)
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
    keybd_event(VK_ESCAPE, static_cast<BYTE>(scan), 0, kOurKey);
    Sleep(40);
    keybd_event(VK_ESCAPE, static_cast<BYTE>(scan), KEYEVENTF_KEYUP, kOurKey);
}

bool game_in_front() {
    DWORD pid = 0;
    GetWindowThreadProcessId(GetForegroundWindow(), &pid);
    return pid == GetCurrentProcessId();
}

// Low-level keyboard hook on its own thread (it needs a message loop): swallows Esc while g_hold_esc is set and the
// game is in front, except the Esc we press ourselves. Every other key passes untouched.
LRESULT CALLBACK keyboard_hook(int code, WPARAM wp, LPARAM lp) {
    if (code == HC_ACTION && (wp == WM_KEYDOWN || wp == WM_SYSKEYDOWN)) {
        const auto* k = reinterpret_cast<const KBDLLHOOKSTRUCT*>(lp);
        if (k->vkCode == 'E' && k->dwExtraInfo != kOurKey && !(k->flags & LLKHF_UP)) g_e_presses.fetch_add(1);
    }
    if (code == HC_ACTION && g_hold_esc.load() && GetTickCount64() - g_hold_seen.load() < 500) {
        const auto* k = reinterpret_cast<const KBDLLHOOKSTRUCT*>(lp);
        if (k->vkCode == VK_ESCAPE && k->dwExtraInfo != kOurKey && game_in_front()) {
            if (wp == WM_KEYDOWN || wp == WM_SYSKEYDOWN) g_esc_held = true;
            return 1;
        }
    }
    return CallNextHookEx(nullptr, code, wp, lp);
}

void start_hook() {
    static std::once_flag once;
    std::call_once(once, [] {
        std::thread([] {
            if (!SetWindowsHookExW(WH_KEYBOARD_LL, keyboard_hook, GetModuleHandleW(nullptr), 0)) {
                log::warn("Activity: keyboard hook not installed ({}); skips will not wait for everyone",
                          GetLastError());
                return;
            }
            MSG m;
            while (GetMessageW(&m, nullptr, 0, 0) > 0) {
            }
        }).detach();
    });
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
void activity_set_versus_test(int presses_per_s) { g_versus_test_rate = presses_per_s; }

namespace {

void write_gauge(uintptr_t a, float v) {
    if (a) *reinterpret_cast<volatile float*>(a) = v;  // a writable float (game::minigame_gauge_address checked it)
}

// Versus arm wrestling (see kPushGain): every tick of the network loop.
void versus_tick(ActivityTick& out, bool connected, bool host, bool cut, const std::string& who, uint32_t now) {
    const uintptr_t ga = game::minigame_gauge_address();
    float g = 0.0f;
    if (ga) g = *reinterpret_cast<volatile float*>(ga);
    const bool live = g_match_on.load() && !cut && ga && g > 0.0f && g <= 100.0f;
    const int test_rate = g_versus_test_rate.load();
    std::lock_guard lock(g_mutex);
    Versus& v = g_vs;
    if (live && !v.live) {
        v.base = g_e_presses.load();
        v.live_since = now;
        v.started = false;
        v.announced = false;
    }
    v.live = live;
    const uint32_t mine = live ? g_e_presses.load() - v.base : 0;
    if (test_rate > 0 && live) {  // one-PC test: a simulated other player in the same match
        v.partner_live = true;
        v.partner_count = static_cast<uint32_t>(static_cast<uint64_t>(now - v.live_since) * test_rate / 1000);
        v.partner_seen = now;
        host = true;
    } else if (!connected || GetTickCount() - v.partner_seen > 1500) {
        v.partner_live = false;
    }
    if ((connected || test_rate > 0) && now - v.last_send >= kVersusSendMs && (live || v.started)) {
        v.last_send = now;
        uint8_t b[9];
        b[0] = live ? 1 : 0;
        std::memcpy(b + 1, &mine, 4);
        std::memcpy(b + 5, &v.gauge, 4);
        if (connected) {
            std::vector<uint8_t> pkt(5 + 9);
            const uint32_t seq = g_seq_out++;
            std::memcpy(pkt.data(), &seq, 4);
            pkt[4] = kVersus;
            std::memcpy(pkt.data() + 5, b, 9);
            capture_emit_activity(std::move(pkt));
        }
    }
    if (!(live && v.partner_live)) {
        if (!live && v.started && !v.announced) {
            // The game ends the match itself near the ends of the gauge (~96 / ~4): the last value tells who won.
            v.announced = true;
            out.toast = v.last_ours > 50.0f ? std::format("You beat {} at arm wrestling!", who)
                                            : std::format("{} won the arm wrestling", who);
            log::info("Activity: versus arm wrestling over: {} ({:.0f})", v.last_ours > 50.0f ? "we won" : "they won",
                      v.last_ours);
        }
        if (!live) v.started = false;
        return;  // a match against the game's NPC, as normal
    }
    if (!v.started) {
        v.started = true;
        log::info("Activity: versus arm wrestling with {} ({})", who, host ? "we hold the gauge" : "they hold the gauge");
    }
    if (host) {
        v.gauge = std::clamp(50.0f + kPushGain * (static_cast<float>(mine) - static_cast<float>(v.partner_count)), 0.0f,
                             100.0f);
        write_gauge(ga, v.gauge);
    } else {
        v.gauge = 100.0f - v.partner_gauge;
        write_gauge(ga, std::clamp(v.gauge, 0.0f, 100.0f));
    }
    const float ours = host ? v.gauge : 100.0f - v.partner_gauge;
    v.last_ours = ours;
    out.banner = std::format("Arm wrestling vs {}: mash E!   You {:.0f} - {:.0f} {}", who, ours, 100.0f - ours, who);
}

}  // namespace

void activity_observe_c2s(const uint8_t* packet, size_t size) {
    if (size < 5) return;
    uint16_t op = 0;
    std::memcpy(&op, packet, 2);
    if (op == kOpActivateMiniGameReq && size >= 5 + 2) {
        uint16_t id = 0;
        std::memcpy(&id, packet + 5, 2);
        log::info("Activity: we started a minigame (id 0x{:04X})", id);
        send(kGameOn, &id, 2);
    } else if (op == kOpSpawnMiniGameCharacterReq) {
        g_match_on = true;
    } else if (op == kOpLeaveMiniGameReq && size >= 5 + 23) {
        g_match_on = false;
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
        case kCutsceneOn: g_partner_cutscene = true; g_partner_cut_pos = pos; g_partner_skip = false; break;
        case kCutsceneOff: g_partner_cutscene = false; g_partner_skip = false; break;
        case kSkip: g_partner_skip = true; g_skip_from = pos; break;
        case kGameOn: g_toasts.push_back("{} is playing a minigame"); break;
        case kVersus:  // u8 live | u32 presses since their match started | f32 gauge (the host's)
            if (p.size() >= 5 + 9) {
                g_vs.partner_live = p[5] != 0;
                std::memcpy(&g_vs.partner_count, p.data() + 6, 4);
                std::memcpy(&g_vs.partner_gauge, p.data() + 10, 4);
                g_vs.partner_seen = GetTickCount();
            }
            break;
        case kGameOff: {
            uint32_t stage = 0;
            if (p.size() >= 9) std::memcpy(&stage, p.data() + 5, 4);
            g_toasts.push_back("{} finished " + activity_minigame_name(stage));
            break;
        }
        default: break;
    }
}

ActivityTick activity_tick(bool connected, bool host, bool focused, std::optional<std::array<float, 3>> me,
                           std::optional<std::array<float, 3>> partner, const std::string& partner_name,
                           uint32_t now) {
    ActivityTick out;
    const bool cut = game::cutscene_playing().value_or(false);
    boss_note_cutscene(cut);
    // Our last known position (none while a minigame holds the player): what we send and measure from.
    static std::array<float, 3> last_here{};
    static bool have_here = false;
    if (me) {
        last_here = *me;
        have_here = true;
    }
    const std::array<float, 3> here = last_here;
    if (!me && have_here) me = last_here;
    if (cut != g_last_cutscene) {
        g_last_cutscene = cut;
        g_cut_since = now;
        g_skip_test_done = false;
        {
            std::lock_guard lock(g_mutex);
            g_our_skip = false;
            g_released = false;
            if (g_skip_test) g_partner_skip = false;
        }
        g_hold_esc = false;
        log::info("Activity: our cutscene {}", cut ? "started" : "ended");
        if (connected) {
            if (cut) send(kCutsceneOn, here.data(), 12);
            else send(kCutsceneOff, nullptr, 0);
        }
    }
    const std::string who = partner_name.empty() ? std::string("Your friend") : partner_name;
    start_hook();
    bool release = false, send_skip = false;
    {
        std::lock_guard lock(g_mutex);
        if (!connected) {
            g_partner_cutscene = false;
            g_partner_skip = false;
            g_toasts.clear();
        }
        if (g_skip_test && cut) {
            // One-PC test: the "other player" is in the same cutscene and presses skip kSkipTestDelayMs in.
            g_partner_cutscene = true;
            g_partner_cut_pos = here;
            if (!g_skip_test_done && now - g_cut_since > kSkipTestDelayMs) {
                g_skip_test_done = true;
                g_partner_skip = true;
                g_skip_from = here;
                log::info("Activity: [dev] skip_test: the other player pressed skip");
            }
        }
        // Everyone in the cutscene must press skip (D32 as the user specified): while the other player is in a
        // cutscene within kNearM, our own Esc is held back (keyboard hook) and only sent to them.
        const bool partner_near = !me || dist_xz(here, g_partner_cut_pos) <= kNearM;
        const bool shared = cut && (connected || g_skip_test) && g_partner_cutscene && partner_near;
        g_hold_esc = shared && !g_released;
        g_hold_seen = GetTickCount64();
        if (g_esc_held.exchange(false) && shared && !g_our_skip) {
            g_our_skip = true;
            send_skip = true;  // sent below, outside g_mutex (send() takes it)
        }
        if (shared && g_our_skip && g_partner_skip && !g_released && focused) {
            g_released = true;
            g_hold_esc = false;
            release = true;
        }
        if (!g_toasts.empty()) {
            out.toast = std::vformat(g_toasts.front(), std::make_format_args(who));
            g_toasts.erase(g_toasts.begin());
        }
        const bool is_near = !partner || !me || dist_xz(*partner, here) <= kNearM;
        if (g_partner_cutscene && is_near && !cut) {
            out.banner = std::format("{} is watching a cutscene", who);
        } else if (shared && g_our_skip && !g_partner_skip) {
            out.banner = std::format("Skip: waiting for {} to press Esc", who);
        } else if (shared && g_partner_skip && !g_our_skip) {
            out.banner = std::format("{} wants to skip: press Esc to skip together", who);
        } else if (shared && !g_released) {
            out.banner = std::format("{} is in this cutscene too: it skips when you both press Esc", who);
        }
    }
    if (send_skip) {
        log::info("Activity: we pressed skip; waiting for the other player");
        send(kSkip, here.data(), 12);
    }
    if (release) {
        press_escape();
        log::info("Activity: both players pressed skip; skipping");
    }
    versus_tick(out, connected, host, cut, who, now);
    return out;
}

}  // namespace cdmp::net
