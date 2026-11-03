#include "mod/net/coop_activity.h"
#include "mod/input/actions.h"
#include "mod/input/pad_input.h"

#include <windows.h>

#include <atomic>
#include <cctype>
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
#include "mod/net/stage_cinema.h"

namespace cdmp::net {
namespace {

constexpr uint16_t kOpActivateMiniGameReq = 2507;  // u16 minigame id | u32 our player
constexpr uint16_t kOpLeaveMiniGameReq = 2379;     // u8 4 | u32 actor | 14 x 0 | u32 stage key
constexpr uint16_t kOpSpawnMiniGameCharacterReq = 3124;  // every match: the NPC opponent is spawned
constexpr float kNearM = 100.0f;                   // the user's rule: cutscenes are shared within 100 m
constexpr uint32_t kSkipTestDelayMs = 6000;

enum Type : uint8_t { kCutsceneOn = 1, kCutsceneOff = 2, kSkip = 3, kGameOn = 4, kGameOff = 5, kVersus = 6, kRps = 10, kChallenge = 11 };

// Versus arm wrestling (D35): while both players are in a match, the host owns one gauge,
// 50 + kPushGain x (host's E presses - guest's E presses), clamped to 0..100, and writes it into its game every tick;
// the guest writes 100 - that into its own. 100 = a win, 0 = a loss: each game then ends its match by itself.
constexpr float kPushGain = 2.0f;
constexpr uint32_t kVersusSendMs = 100;
std::atomic<uint32_t> g_e_presses{0};     // E key-downs (the hook counts them; not ours)
std::atomic<bool> g_match_on{false};      // between SpawnMiniGameCharacterReq and LeaveMiniGameReq
std::atomic<uint32_t> g_match_stage{0};   // the match's minigame stage key (0 = unknown)

// The minigames that use the strength gauge the versus shares (arm wrestling's stages, hand wrestling). Any other
// minigame keeps its own rules: the versus never writes into it.
bool is_gauge_game(uint32_t stage) {
    switch (stage) {
        case 16005100: case 1001987: case 1001988: case 1003873: case 1003875: case 1018693: return true;
        default: return false;
    }
}
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

// Rock, paper, scissors duel (D39). The town RPS minigame cannot be turned into player against player: the child's
// hand comes from the stage's server-side random roll (AddStageRandomRateAck), and the server refuses an outcome that
// does not match it (2026-11-02: a rewritten pick transition got no answer). So an RPS challenge is a duel the mod runs
// itself, anywhere: each player presses U (scissors) / F (paper) / E (rock), the keyboard hook keeps those keys from
// the game meanwhile, both hands are shown at once when both have picked, best of three, then the next match.
constexpr uint32_t kRpsTestDelayMs = 4000;   // [dev] rps_test: the simulated player picks this long after us
constexpr uint32_t kRpsRevealMs = 3500;      // both hands on screen before the next round
constexpr uint32_t kRpsNameStage = 1001949;  // Hernand_MiniGame_RockPaperSci: names the game in messages
constexpr int kRpsKeys[3] = {'U', 'F', 'E'};  // scissors, paper, rock
const char* const kHands[3] = {"scissors", "paper", "rock"};
std::atomic<bool> g_rps_hold{false};  // the hook keeps U/F/E from the game (a duel is waiting for our hand)
std::atomic<int> g_rps_pressed{-1};   // the hand the hook caught (-1 none)
std::atomic<int> g_rps_test{-1};      // [dev] rps_test: a simulated other player always showing this hand
struct Rps {
    int ours = -1, theirs = -1;  // this round's hands (-1 not yet)
    uint32_t picked_at = 0;
    uint32_t reveal_until = 0;   // both hands shown until then
    std::string reveal;          // what is shown
} g_rps;

// Challenges (D40, user 2026-11-02: "give me the option to challenge a friend in the mini games"). Versus play only
// happens inside an accepted challenge: Shift+F5 invites the other player, to arm / hand wrestling when standing at
// such a table, otherwise to a rock-paper-scissors duel (anywhere). They accept with Shift+F5 (Shift+F6
// declines; an invite lapses after a minute). Wrestling is played at the same table in each world, the shared gauge
// above making it player against player; the duel runs in the mod. A running series score is kept until either player
// ends the challenge (Shift+F5 again).
// Event kChallenge: u8 action | u32 the game's stage key.
enum Game : uint8_t { kNoGame = 0, kGameWrestle = 1, kGameRps = 2 };
enum ChallengeAction : uint8_t { kInvite = 1, kAccept = 2, kDecline = 3, kEnd = 4 };
enum ChallengeState : uint8_t { kChNone, kChSent, kChReceived, kChOn };
constexpr uint32_t kWrestleStages[] = {16005100, 1001987, 1001988, 1003873, 1003875, 1018693};
constexpr float kChallengeNearM = 30.0f;  // how close to the table Shift+F5 finds it
constexpr uint32_t kInviteMs = 60000;
struct Challenge {
    ChallengeState state = kChNone;
    uint32_t stage = 0;          // the game's stage key
    uint32_t since = 0;
    int ours = 0, theirs = 0;    // series: matches won
    int round_ours = 0, round_theirs = 0;  // RPS: rounds won in the current match (first to 2)
} g_ch;
// [dev] challenge_test: 1 = a simulated other player accepts our invites; 2 = they also challenge us to a duel once.
std::atomic<int> g_challenge_test{0};

Game game_of(uint32_t stage);

// A text that goes through std::vformat later (g_toasts): its literal braces (button tokens) doubled.
std::string escape_braces(const std::string& t) {
    std::string r;
    for (char ch : t) {
        r += ch;
        if (ch == '{' || ch == '}') r += ch;
    }
    return r;
}

// What to do once a challenge is on.
std::string how_to_play(uint32_t stage) {
    return game_of(stage) == kGameRps ? "Pick your hand:  {RpsScissors} scissors  {RpsPaper} paper  {RpsRock} rock  (first to 2)"
                                      : "Both sit at the table and start a match, then mash {Mash}!";
}

Game game_of(uint32_t stage) {
    if (std::find(std::begin(kWrestleStages), std::end(kWrestleStages), stage) != std::end(kWrestleStages)) return kGameWrestle;
    if (stage == kRpsNameStage) return kGameRps;
    return kNoGame;
}

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
    if (code == HC_ACTION && g_rps_hold.load() && GetTickCount64() - g_hold_seen.load() < 500) {
        const auto* k = reinterpret_cast<const KBDLLHOOKSTRUCT*>(lp);
        for (int hand = 0; hand < 3; ++hand) {
            if (k->vkCode == static_cast<DWORD>(kRpsKeys[hand]) && game_in_front()) {
                if (wp == WM_KEYDOWN || wp == WM_SYSKEYDOWN) g_rps_pressed = hand;
                return 1;
            }
        }
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

void activity_send(uint8_t type, const std::vector<uint8_t>& data) { send(static_cast<Type>(type), data.data(), data.size()); }

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
    const bool live = g_match_on.load() && is_gauge_game(g_match_stage.load()) && !cut && ga && g > 0.0f && g <= 100.0f;
    const std::string game = activity_minigame_name(g_match_stage.load());
    const int test_rate = g_versus_test_rate.load();
    std::lock_guard lock(g_mutex);
    // Only inside an accepted wrestling challenge (or the one-PC test): otherwise a match against the NPC, as normal.
    const bool challenged = (g_ch.state == kChOn && game_of(g_ch.stage) == kGameWrestle) || test_rate > 0;
    Versus& v = g_vs;
    if (live && !v.live) {
        v.base = g_e_presses.load();
        v.live_since = now;
        v.started = false;
        v.announced = false;
    }
    v.live = live;
    const uint32_t mine = live ? g_e_presses.load() - v.base : 0;
    if (!challenged) {
        v.partner_live = false;
    } else if (test_rate > 0 && live) {  // one-PC test: a simulated other player in the same match
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
            (v.last_ours > 50.0f ? g_ch.ours : g_ch.theirs) += 1;
            out.toast = std::format("{}   Series: you {} - {} {}",
                                    v.last_ours > 50.0f ? std::format("You beat {} at {}!", who, game)
                                                        : std::format("{} won the {}", who, game),
                                    g_ch.ours, g_ch.theirs, who);
            log::info("Activity: versus {} over: {} ({:.0f})", game, v.last_ours > 50.0f ? "we won" : "they won",
                      v.last_ours);
        }
        if (!live) v.started = false;
        return;  // a match against the game's NPC, as normal
    }
    if (!v.started) {
        v.started = true;
        log::info("Activity: versus {} with {} ({})", game, who, host ? "we hold the gauge" : "they hold the gauge");
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
    std::string title = game;
    if (!title.empty()) title[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(title[0])));
    out.banner = std::format("{} vs {}: mash {{Mash}}!   You {:.0f} - {:.0f} {}", title, who, ours, 100.0f - ours, who);
}

// The pad's side of what the keyboard hook catches (input/actions.h): mash presses, RPS hands, the cutscene skip.
// XInput pads' buttons are kept from the game while a Co-op+ box wants them.
void pad_tick() {
    static input::Edge mash, scissors, paper, rock, skip;
    if (mash(input::action_down(input::Action::Mash, false)) && g_match_on.load()) g_e_presses.fetch_add(1);
    const bool rps = g_rps_hold.load() && GetTickCount64() - g_hold_seen.load() < 500;
    if (rps) input::mask_from_game(input::kPadX | input::kPadY | input::kPadB, 150);
    const bool s = scissors(input::action_down(input::Action::RpsScissors, false));
    const bool p = paper(input::action_down(input::Action::RpsPaper, false));
    const bool r = rock(input::action_down(input::Action::RpsRock, false));
    if (rps && (s || p || r)) g_rps_pressed = s ? 0 : p ? 1 : 2;
    const bool hold = g_hold_esc.load() && GetTickCount64() - g_hold_seen.load() < 500;
    if (hold) input::mask_from_game(input::kPadStart | input::kPadB, 150);
    if (skip(input::action_down(input::Action::Skip, false)) && hold) g_esc_held = true;
}

// The rock-paper-scissors duel (see Rps): every tick of the network loop.
void rps_tick(ActivityTick& out, bool connected, const std::string& who) {
    const uint32_t now = GetTickCount();
    const int test = g_rps_test.load();
    int send_hand = -1;
    {
        std::lock_guard lock(g_mutex);
        Rps& r = g_rps;
        Challenge& c = g_ch;
        const bool on = c.state == kChOn && game_of(c.stage) == kGameRps;
        if (!on) {
            g_rps_hold = false;
            g_rps_pressed = -1;
            r = Rps{};
            return;
        }
        const bool picking = static_cast<int32_t>(now - r.reveal_until) >= 0;
        g_rps_hold = picking;
        const int pressed = g_rps_pressed.exchange(-1);
        if (picking && pressed >= 0 && r.ours < 0) {
            r.ours = pressed;
            r.picked_at = now;
            send_hand = pressed;
            log::info("Activity: rock, paper, scissors: we picked {}", kHands[pressed]);
        }
        if (test >= 0 && r.ours >= 0 && r.theirs < 0 && now - r.picked_at > kRpsTestDelayMs) r.theirs = test;
        if (picking && r.ours >= 0 && r.theirs >= 0) {
            // Scissors 0 beats paper 1, paper 1 beats rock 2, rock 2 beats scissors 0: a beats b when a == (b + 2) % 3.
            const bool win = r.ours == (r.theirs + 2) % 3, lose = r.theirs == (r.ours + 2) % 3;
            if (win) ++c.round_ours;
            if (lose) ++c.round_theirs;
            r.reveal = std::format("You: {}   {}: {}   -   {}", kHands[r.ours], who, kHands[r.theirs],
                                   win ? std::string("you win the round") : lose ? who + " wins the round" : "a draw");
            log::info("Activity: rock, paper, scissors: {} vs {} ({})", kHands[r.ours], kHands[r.theirs],
                      win ? "won" : lose ? "lost" : "draw");
            if (c.round_ours == 2 || c.round_theirs == 2) {
                const bool match = c.round_ours == 2;
                (match ? c.ours : c.theirs) += 1;
                out.toast = std::format("{} rock, paper, scissors {} - {}!   Series: you {} - {} {}",
                                        match ? std::string("You win") : who + " wins", std::max(c.round_ours, c.round_theirs),
                                        std::min(c.round_ours, c.round_theirs), c.ours, c.theirs, who);
                log::info("Activity: rock, paper, scissors match: {} ({} - {})", match ? "we won" : "they won",
                          c.round_ours, c.round_theirs);
                c.round_ours = c.round_theirs = 0;
            }
            r.ours = r.theirs = -1;
            r.reveal_until = now + kRpsRevealMs;
            g_rps_hold = false;
        }
        const std::string score = std::format("you {} - {} {}", c.round_ours, c.round_theirs, who);
        if (!picking) {
            out.banner = r.reveal + "   (" + score + ")";
        } else if (r.ours < 0) {
            out.banner = std::format("Rock, paper, scissors vs {} ({}, first to 2):   {{RpsScissors}} scissors   {{RpsPaper}} paper   {{RpsRock}} rock{}", who,
                                     score, r.theirs >= 0 ? std::format("   ({} has picked)", who) : "");
        } else {
            out.banner = std::format("Rock, paper, scissors: you picked {}; waiting for {}   ({})", kHands[r.ours], who, score);
        }
    }
    if (send_hand >= 0 && connected) {
        const uint8_t h = static_cast<uint8_t>(send_hand);
        send(kRps, &h, 1);
    }
}

// Challenges (see Challenge): Shift+F5 / Shift+F6, invites lapsing, the RPS child made willing, banners.
void challenge_tick(ActivityTick& out, bool connected, bool focused, std::optional<std::array<float, 3>> me,
                    const std::string& who) {
    const uint32_t now = GetTickCount();
    // Shift, not Ctrl: Ctrl is a game key (it moves the interaction focus off the table host, and crouches).
    // Shift+F5 / Shift+F6, or on a pad the co-op chord + D-pad Up / Down (input/actions.h).
    static input::Edge challenge_edge, decline_edge;
    const bool pressed = challenge_edge(input::action_down(input::Action::Challenge, focused));
    bool declined = decline_edge(input::action_down(input::Action::Decline, focused));
    const int test_mode = g_challenge_test.load();
    const bool test = test_mode != 0;
    std::optional<std::pair<uint8_t, uint32_t>> send_it;
    std::string toast;
    {
        std::lock_guard lock(g_mutex);
        Challenge& c = g_ch;
        if (!connected && !test && c.state != kChNone) {
            c = Challenge{};
            toast = "Challenge ended: the other player left";
        }
        if ((c.state == kChSent || c.state == kChReceived) && now - c.since > kInviteMs) {
            toast = c.state == kChSent ? std::format("{} did not answer the challenge", who) : "The challenge lapsed";
            c.state = kChNone;
        }
        if (test_mode == 2 && declined && c.state == kChNone) {  // [dev] Shift+F6: the simulated player invites us
            c.state = kChReceived;
            c.stage = kRpsNameStage;
            c.since = now;
            toast = std::format("{} challenges you to rock, paper, scissors! {{Challenge}} to accept", who);
            log::info("Activity: [dev] challenge_test: the other player challenges us");
            declined = false;  // that press was the trigger
        }
        if (test && c.state == kChSent && now - c.since > 2000) {  // [dev] the simulated player accepts
            c.state = kChOn;
            toast = std::format("{} accepted: {}! {}", who, activity_minigame_name(c.stage), how_to_play(c.stage));
            log::info("Activity: [dev] challenge_test: the other player accepted");
        }
        if (declined && c.state == kChReceived) {
            send_it = std::make_pair(static_cast<uint8_t>(kDecline), c.stage);
            c.state = kChNone;
            toast = "Challenge declined";
        } else if (pressed) {
            switch (c.state) {
                case kChReceived:
                    c.state = kChOn;
                    c.ours = c.theirs = c.round_ours = c.round_theirs = 0;
                    send_it = std::make_pair(static_cast<uint8_t>(kAccept), c.stage);
                    toast = std::format("Challenge accepted: {} vs {}. {}", activity_minigame_name(c.stage), who,
                                        how_to_play(c.stage));
                    break;
                case kChSent:
                case kChOn:
                    send_it = std::make_pair(static_cast<uint8_t>(kEnd), c.stage);
                    toast = c.state == kChOn
                                ? std::format("Challenge over. Final series: you {} - {} {}", c.ours, c.theirs, who)
                                : std::string("Challenge withdrawn");
                    c = Challenge{};
                    break;
                case kChNone: {
                    // At a wrestling table: that game; anywhere else: a rock-paper-scissors duel.
                    uint32_t found = me ? cinema_minigame_near(*me, kChallengeNearM, kWrestleStages,
                                                               std::size(kWrestleStages))
                                        : 0;
                    if (!found) found = kRpsNameStage;
                    if (!connected && !test) {
                        toast = "No one to challenge: invite a friend first ({Invite})";
                    } else {
                        c = Challenge{};
                        c.state = kChSent;
                        c.stage = found;
                        c.since = now;
                        send_it = std::make_pair(static_cast<uint8_t>(kInvite), found);
                        toast = std::format("You challenged {} to {}", who, activity_minigame_name(found));
                    }
                    break;
                }
            }
        }
        if (out.banner.empty()) {
            const std::string game = activity_minigame_name(c.stage);
            if (c.state == kChReceived) {
                out.banner = std::format("{} challenges you to {}!   {{Challenge}} accept   {{Decline}} decline", who, game);
            } else if (c.state == kChSent) {
                out.banner = std::format("Challenge sent: {} vs {}. Waiting for an answer ({{Challenge}} withdraws)", game, who);
            } else if (c.state == kChOn) {
                out.banner = std::format("Challenge: {} vs {}   series you {} - {} {}   (start a match at the table; "
                                         "{{Challenge}} ends)", game, who, c.ours, c.theirs, who);
            }
        }
    }
    if (send_it) {
        std::vector<uint8_t> b(5);
        b[0] = send_it->first;
        std::memcpy(b.data() + 1, &send_it->second, 4);
        send(kChallenge, b.data(), b.size());
        log::info("Activity: challenge action {} sent ({})", static_cast<int>(send_it->first),
                  activity_minigame_name(send_it->second));
    }
    if (!toast.empty() && out.toast.empty()) out.toast = std::move(toast);
}

}  // namespace

void activity_set_challenge_test(int mode) { g_challenge_test = mode; }
void activity_set_rps_test(int hand) { g_rps_test = hand; }

void activity_observe_c2s(const uint8_t* packet, size_t size) {
    if (size < 5) return;
    uint16_t op = 0;
    std::memcpy(&op, packet, 2);
    if (op == kOpActivateMiniGameReq && size >= 5 + 2) {
        uint16_t id = 0;
        std::memcpy(&id, packet + 5, 2);
        log::info("Activity: we started a minigame (id 0x{:04X})", id);
        send(kGameOn, &id, 2);
    } else if (op == kOpSpawnMiniGameCharacterReq && size >= 5 + 9) {
        // u8 4 | u64 the minigame's stage (sequencer) | u16 id | u32 opponent | ...
        uint64_t seq = 0;
        std::memcpy(&seq, packet + 6, 8);
        g_match_stage = cinema_stage_key(seq);
        g_match_on = true;
        log::info("Activity: a match of {} (stage {})", activity_minigame_name(g_match_stage.load()), g_match_stage.load());
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
        case 7: case 8: case 9:  // stage_cinema.h (it takes its own lock)
            break;
        case kChallenge:  // u8 action | u32 stage key
            if (p.size() >= 10) {
                uint32_t stage = 0;
                std::memcpy(&stage, p.data() + 6, 4);
                const std::string game = activity_minigame_name(stage);
                switch (p[5]) {
                    case kInvite:
                        g_ch = Challenge{};
                        g_ch.state = kChReceived;
                        g_ch.stage = stage;
                        g_ch.since = GetTickCount();
                        g_toasts.push_back("{} challenges you to " + game + "! {{Challenge}} to accept");
                        break;
                    case kAccept:
                        if (g_ch.state == kChSent) {
                            g_ch.state = kChOn;
                            g_toasts.push_back("{} accepted: " + game + "! " + escape_braces(how_to_play(stage)));
                        }
                        break;
                    case kDecline:
                        if (g_ch.state == kChSent) g_ch.state = kChNone;
                        g_toasts.push_back("{} declined the challenge");
                        break;
                    case kEnd:
                        if (g_ch.state != kChNone) {
                            g_toasts.push_back(std::format("{{}} ended the challenge. Final series: you {} - {}", g_ch.ours,
                                                           g_ch.theirs));
                        }
                        g_ch = Challenge{};
                        break;
                    default: break;
                }
            }
            break;
        case kRps:  // u8 hand (0 scissors, 1 paper, 2 rock): their pick in the duel
            if (p.size() >= 6 && p[5] < 3) g_rps.theirs = p[5];
            break;
        default: break;
    }
    if (p[4] >= 7 && p[4] <= 9) cinema_on_event(p[4], p.data() + 5, p.size() - 5);
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
    pad_tick();
    cinema_tick(connected, host, cut, me, partner, now);  // the host's stage cinematics for the guest (D38)
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
            out.banner = std::format("Skip: waiting for {}", who);
        } else if (shared && g_partner_skip && !g_our_skip) {
            out.banner = std::format("{} wants to skip: {{Skip}} to skip together", who);
        } else if (shared && !g_released) {
            out.banner = std::format("{} is in this cutscene too: it skips when you both press {{Skip}}", who);
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
    rps_tick(out, connected, who);
    challenge_tick(out, connected, focused, me, who);
    return out;
}

}  // namespace cdmp::net
