#include "mod/net/coop_lobby.h"

#include <windows.h>

#include <atomic>
#include <cstring>
#include <format>
#include <mutex>

#include "mod/log.h"
#include "mod/net/coop_worlds.h"
#include "mod/net/troc_sniffer.h"

namespace cdmp::net {
namespace {

// RelKind::World message types (coop_worlds.cpp keeps 1 for its own; 2.. come here).
constexpr uint8_t kMsgGo = 3;          // u8 | u8 new game | u16 ms to start      host -> guest
constexpr uint8_t kMsgFresh = 4;       // u8                                       host -> guest: a fresh co-op save
constexpr uint8_t kMsgDifficulty = 5;  // u8 | u32 difficulty                      either way
constexpr uint8_t kMsgPlan = 6;        // u8 | u8 LobbyPlan                        host -> guest
constexpr uint64_t kCountdownMs = 3000;
constexpr uint64_t kDifficultyWaitMs = 25000;  // the guest's new game waits this long for the host's difficulty
constexpr uint32_t kUnknown = UINT32_MAX;
constexpr uint32_t kNormal = 1;  // 0 Easy, 1 Normal, 2 Hard (the game's dialog, top to bottom)

enum class Phase { Menu, HostPrepare, HostSetup, Starting, GuestWait };

std::mutex g_m;
LobbyInput g_in;
bool g_dismissed = false, g_was_at_title = false, g_choosing = false;
Phase g_phase = Phase::Menu;
LobbyPlan g_plan = LobbyPlan::None;
uint64_t g_go_at = 0;            // countdown end (both games press Play)
bool g_go_sent = false;          // host: the guest was told the run started
uint64_t g_new_go_since = 0;     // guest: the host's run started at (new save): our New Game follows
bool g_new_started = false;      // guest: our New Game was pressed
uint32_t g_gen_mark = 0;         // the run generation when the phase began: a later one = our run entered the world
std::vector<std::vector<uint8_t>> g_out;
std::atomic<uint32_t> g_own_diff{kUnknown};
uint32_t g_partner_diff = kUnknown, g_sent_diff = kUnknown;
std::atomic<uint16_t> g_diff_op{0};

uint64_t now_ms() { return GetTickCount64(); }
void send(std::vector<uint8_t> m) { g_out.push_back(std::move(m)); }

void reset_phase() {
    g_phase = Phase::Menu;
    g_plan = LobbyPlan::None;
    g_go_at = 0;
    g_go_sent = false;
    g_new_go_since = 0;
    g_new_started = false;
    g_choosing = false;
}

const char* plan_text(LobbyPlan p) {
    switch (p) {
        case LobbyPlan::NewSave: return "a new co-op save";
        case LobbyPlan::ContinueCoop: return "your co-op save";
        case LobbyPlan::FromSolo: return "the host's solo progress";
        default: return "nothing yet";
    }
}

}  // namespace

void lobby_tick(const LobbyInput& in) {
    const WorldsLobbyInfo w = worlds_lobby_info();
    const bool save_ready = !w.enabled || (w.active && !w.switching);
    enum { None, PressNew, PressPlay, GuestNew } todo = None;
    uint32_t guest_diff = kNormal;
    {
        std::lock_guard lock(g_m);
        if (in.at_title && !g_was_at_title) g_dismissed = false;  // back at the title: the menu again
        g_was_at_title = in.at_title;
        if (!in.connected) {
            if (g_phase != Phase::Menu && g_phase != Phase::HostSetup) log::info("Co-op lobby: the party broke up");
            if (g_phase != Phase::HostSetup) reset_phase();
            g_partner_diff = g_sent_diff = kUnknown;
        }
        // Our run entered the world (a new character capture since the phase began): the lobby's work is done. The host
        // in its setup first tells the guest (HostSetup below).
        if (g_phase != Phase::Menu && g_phase != Phase::HostSetup && in.run_generation > g_gen_mark) reset_phase();
        g_in = in;
        const uint32_t own = g_own_diff.load();
        if (in.connected && own != kUnknown && own != g_sent_diff) {
            std::vector<uint8_t> m{kMsgDifficulty, 0, 0, 0, 0};
            std::memcpy(m.data() + 1, &own, 4);
            send(std::move(m));
            g_sent_diff = own;
        }
        switch (g_phase) {
            case Phase::HostPrepare:
                // Ready when the co-op save the host chose is the one in use (a new one: its new game not begun yet).
                if (save_ready && in.at_title && (g_plan != LobbyPlan::NewSave || w.new_game_pending)) {
                    if (g_plan == LobbyPlan::NewSave || w.new_game_pending) {
                        g_phase = Phase::HostSetup;
                        g_gen_mark = in.run_generation;
                        todo = PressNew;
                        log::info("Co-op lobby: our co-op save is ready: New Game (the host picks the difficulty)");
                    } else {
                        g_phase = Phase::Starting;
                        g_go_at = in.now_ms + kCountdownMs;
                        std::vector<uint8_t> m{kMsgGo, 0, 0, 0};
                        const uint16_t ms = static_cast<uint16_t>(kCountdownMs);
                        std::memcpy(m.data() + 2, &ms, 2);
                        send(std::move(m));
                        log::info("Co-op lobby: both games start in {} ms", ms);
                    }
                }
                break;
            case Phase::HostSetup:
                // The host went through the game's own setup (agreement, difficulty): when its run enters the world, the
                // guest's game follows.
                if (in.run_generation > g_gen_mark) {
                    send({kMsgGo, 1, 0, 0});
                    log::info("Co-op lobby: our run entered the world: the guest's game follows");
                    reset_phase();
                }
                break;
            case Phase::Starting:
                if (g_go_at && in.now_ms >= g_go_at) {
                    g_go_at = 0;
                    g_phase = Phase::Menu;
                    todo = PressPlay;
                }
                break;
            case Phase::GuestWait:
                if (g_go_at && in.now_ms >= g_go_at) {
                    g_go_at = 0;
                    todo = PressPlay;
                } else if (g_new_go_since && !g_new_started && save_ready && in.at_title &&
                           (g_partner_diff != kUnknown || in.now_ms - g_new_go_since > kDifficultyWaitMs)) {
                    g_new_started = true;
                    guest_diff = g_partner_diff != kUnknown && g_partner_diff <= 2 ? g_partner_diff : kNormal;
                    todo = GuestNew;
                    log::info("Co-op lobby: the host's run started: our New Game on difficulty {}", guest_diff);
                }
                break;
            case Phase::Menu:
                break;
        }
    }
    if (todo == PressNew) worlds_lobby_press(true);
    if (todo == PressPlay) worlds_lobby_press(false);
    if (todo == GuestNew) worlds_lobby_guest_new(static_cast<int>(guest_diff));
}

LobbyView lobby_view() {
    const WorldsLobbyInfo w = worlds_lobby_info();
    std::lock_guard lock(g_m);
    LobbyView v;
    v.at_title = g_in.at_title;
    v.host = g_in.host;
    v.connected = g_in.connected;
    v.worlds_on = w.enabled;
    v.own_name = g_in.own_name.empty() ? "You" : g_in.own_name;
    v.partner_name = g_in.partner_name.empty() ? "your friend" : g_in.partner_name;
    v.can_continue = w.exists && w.has_saves && !w.new_game_pending;
    v.plan = g_plan;
    v.choosing = g_choosing;
    v.countdown = g_go_at ? static_cast<int>((g_go_at - std::min(g_go_at, now_ms()) + 999) / 1000) : -1;
    v.guest_wait = !g_in.host && g_phase == Phase::GuestWait;
    v.show = g_in.at_title && !g_dismissed && g_phase != Phase::HostSetup && !v.guest_wait;
    const std::string& f = v.partner_name;
    if (v.guest_wait) {
        v.wait_line = g_new_started ? "Joining the start of the story..."
                      : g_plan == LobbyPlan::NewSave
                          ? f + " is setting up the game. You join at the start of the story."
                          : f + " is getting ready. You both start together.";
    }
    if (v.countdown >= 0) {
        v.status = "Both games start together...";
    } else if (g_phase == Phase::HostPrepare) {
        v.status = "Preparing your co-op save with " + f + "...";
    } else if (v.host) {
        v.status = v.connected ? f + " is in your party. Start Co-op when you are both ready."
                               : "Invite a friend to begin a co-op journey, or start solo.";
    } else {
        v.status = v.connected ? "You are in " + f + "'s party. " + f + " starts the journey."
                               : "Waiting for the host: accept their Steam invite, or use Join Game on them in Steam.";
    }
    return v;
}

void lobby_start_coop() {
    std::lock_guard lock(g_m);
    if (!g_in.host || !g_in.connected || g_phase != Phase::Menu) return;
    g_choosing = true;
}

void lobby_choose(LobbyPlan plan) {
    {
        std::lock_guard lock(g_m);
        g_choosing = false;
        if (plan == LobbyPlan::None || !g_in.host || !g_in.connected) return;
    }
    // The co-op save first (the network loop must not see the old one as ready), then the phase.
    std::string why;
    bool ok = true;
    if (plan == LobbyPlan::NewSave) ok = worlds_lobby_fresh(&why);
    else if (plan == LobbyPlan::FromSolo) ok = worlds_lobby_from_solo(&why);
    else worlds_lobby_continue();
    if (!ok) log::warn("Co-op lobby: the co-op save could not be prepared: {}", why);
    std::lock_guard lock(g_m);
    g_plan = plan;
    g_phase = Phase::HostPrepare;
    g_gen_mark = g_in.run_generation;
    send({kMsgPlan, static_cast<uint8_t>(plan)});
    if (plan == LobbyPlan::NewSave) send({kMsgFresh});
    log::info("Co-op lobby: the host chose {}", plan_text(plan));
}

void lobby_dismiss() {
    std::lock_guard lock(g_m);
    g_dismissed = true;
    g_choosing = false;
    log::info("Co-op lobby: closed (the game's own title)");
}

void lobby_reopen() {
    std::lock_guard lock(g_m);
    g_dismissed = false;
}

std::vector<std::vector<uint8_t>> lobby_take_outgoing() {
    std::lock_guard lock(g_m);
    std::vector<std::vector<uint8_t>> out;
    out.swap(g_out);
    return out;
}

void lobby_on_message(const std::vector<uint8_t>& b) {
    if (b.empty()) return;
    bool fresh = false;
    {
        std::lock_guard lock(g_m);
        switch (b[0]) {
            case kMsgPlan:
                if (b.size() >= 2 && !g_in.host) {
                    reset_phase();
                    g_plan = static_cast<LobbyPlan>(b[1]);
                    g_phase = Phase::GuestWait;
                    g_gen_mark = g_in.run_generation;
                    log::info("Co-op lobby: the host chose {}: waiting for the host", plan_text(g_plan));
                }
                break;
            case kMsgGo:
                if (b.size() >= 4 && !g_in.host) {
                    if (b[1]) {
                        g_new_go_since = now_ms();
                        log::info("Co-op lobby: the host's run started");
                    } else {
                        uint16_t ms = 0;
                        std::memcpy(&ms, b.data() + 2, 2);
                        g_go_at = now_ms() + std::min<uint16_t>(ms, 10000);
                        log::info("Co-op lobby: the host starts in {} ms", ms);
                    }
                    if (g_phase != Phase::GuestWait) g_gen_mark = g_in.run_generation;
                    g_phase = Phase::GuestWait;
                }
                break;
            case kMsgFresh:
                fresh = !g_in.host;
                break;
            case kMsgDifficulty:
                if (b.size() >= 5) {
                    std::memcpy(&g_partner_diff, b.data() + 1, 4);
                    log::info("Co-op lobby: {}'s difficulty is {}", g_in.partner_name, g_partner_diff);
                }
                break;
        }
    }
    if (fresh) {
        std::string why;
        if (!worlds_lobby_fresh(&why)) log::warn("Co-op lobby: the host's fresh save could not start here: {}", why);
    }
}

void lobby_observe_c2s(const uint8_t* packet, size_t size) {
    uint16_t op = g_diff_op.load(std::memory_order_relaxed);
    if (!op) {
        op = opcode_for("ChangeGameDifficultyReq", 0xFFFF);
        g_diff_op = op;
    }
    if (size < 9 || op == 0xFFFF || static_cast<uint16_t>(packet[0] | (packet[1] << 8)) != op) return;
    uint32_t d = 0;
    std::memcpy(&d, packet + 5, 4);
    if (d != g_own_diff.exchange(d)) log::info("Co-op lobby: our difficulty is {}", d);
}

std::string lobby_difficulty_card() {
    const uint32_t own = g_own_diff.load();
    std::lock_guard lock(g_m);
    if (!g_in.connected || !g_in.in_world || own == kUnknown || g_partner_diff == kUnknown || own == g_partner_diff) return {};
    static const char* const kNames[] = {"Easy", "Normal", "Hard"};
    const auto name = [](uint32_t d) { return d <= 2 ? std::string(kNames[d]) : std::format("difficulty {}", d + 1); };
    return std::format("Choose the same difficulty\nYou play on {} and {} on {}. Pick the same one in the game's "
                       "Settings so the fights are as hard for both of you.",
                       name(own), g_in.partner_name.empty() ? "your friend" : g_in.partner_name, name(g_partner_diff));
}

}  // namespace cdmp::net
