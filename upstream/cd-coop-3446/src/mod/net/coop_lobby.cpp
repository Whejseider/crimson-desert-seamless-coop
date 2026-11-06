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
constexpr uint8_t kMsgReady = 2;       // u8 | u8 ready                      guest -> host
constexpr uint8_t kMsgGo = 3;          // u8 | u8 new game | u16 ms to start  host -> guest
constexpr uint8_t kMsgFresh = 4;       // u8                                  host -> guest: a fresh co-op save
constexpr uint8_t kMsgDifficulty = 5;  // u8 | u32 difficulty                 either way
constexpr uint64_t kCountdownMs = 3000;
constexpr uint32_t kUnknown = UINT32_MAX;

std::mutex g_m;
LobbyInput g_in;
bool g_dismissed = false, g_was_at_title = false;
bool g_we_ready = false, g_partner_ready = false;
uint64_t g_go_at = 0;
bool g_go_new = false;
std::vector<std::vector<uint8_t>> g_out;
std::atomic<uint32_t> g_own_diff{kUnknown};
uint32_t g_partner_diff = kUnknown, g_sent_diff = kUnknown;
std::atomic<uint16_t> g_diff_op{0};

uint64_t now_ms() { return GetTickCount64(); }

void send(std::vector<uint8_t> m) { g_out.push_back(std::move(m)); }

}  // namespace

void lobby_tick(const LobbyInput& in) {
    bool press = false, press_new = false;
    {
        std::lock_guard lock(g_m);
        if (in.at_title && !g_was_at_title) g_dismissed = false;  // back at the title: the menu again
        g_was_at_title = in.at_title;
        if (!in.connected) {
            g_we_ready = g_partner_ready = false;
            g_go_at = 0;
            g_partner_diff = g_sent_diff = kUnknown;
        }
        g_in = in;
        const uint32_t own = g_own_diff.load();
        if (in.connected && own != kUnknown && own != g_sent_diff) {
            std::vector<uint8_t> m{kMsgDifficulty, 0, 0, 0, 0};
            std::memcpy(m.data() + 1, &own, 4);
            send(std::move(m));
            g_sent_diff = own;
        }
        if (g_go_at && in.now_ms >= g_go_at) {
            g_go_at = 0;
            press = in.at_title;
            press_new = g_go_new;
        }
    }
    if (press) {
        const WorldsLobbyInfo w = worlds_lobby_info();
        worlds_lobby_press(press_new || w.new_game_pending);
    }
}

LobbyView lobby_view() {
    const WorldsLobbyInfo w = worlds_lobby_info();
    std::lock_guard lock(g_m);
    LobbyView v;
    v.show = g_in.at_title && !g_dismissed && !w.switching;
    v.at_title = g_in.at_title;
    v.host = g_in.host;
    v.connected = g_in.connected;
    v.worlds_on = w.enabled;
    v.own_name = g_in.own_name.empty() ? "You" : g_in.own_name;
    v.partner_name = g_in.partner_name.empty() ? "your friend" : g_in.partner_name;
    v.partner_ready = g_partner_ready;
    v.we_ready = g_we_ready;
    v.can_continue = w.exists && w.has_saves && !w.new_game_pending;
    v.choice = w.choice == save_io::WorldStart::NewGame ? 1 : w.choice == save_io::WorldStart::Continue ? 2 : 0;
    if (!w.enabled) v.choice = 2;  // no co-op saves: each plays on in their own save
    v.save_ready = !w.enabled || (w.active && !w.switching);
    v.countdown = g_go_at ? static_cast<int>((g_go_at - std::min(g_go_at, now_ms()) + 999) / 1000) : -1;
    v.difficulty_mismatch = g_own_diff.load() != kUnknown && g_partner_diff != kUnknown && g_own_diff.load() != g_partner_diff;
    const std::string& f = v.partner_name;
    if (v.countdown >= 0) {
        v.status = "Both games start together...";
    } else if (v.host) {
        if (!v.connected) v.status = "Invite a friend to play together, or play solo.";
        else if (v.choice == 0) v.status = "Choose how you and " + f + " begin.";
        else if (!v.save_ready) v.status = "Preparing your co-op save with " + f + "...";
        else if (!v.partner_ready) v.status = "Waiting for " + f + " to ready up.";
        else v.status = f + " is ready. Start when you are.";
    } else {
        if (!v.connected) v.status = "Waiting for the host: accept their Steam invite, or use Join Game on them in Steam.";
        else if (v.choice == 0) v.status = "Waiting for " + f + " to choose how you begin.";
        else if (!v.save_ready) v.status = "Preparing your co-op save with " + f + "...";
        else v.status = std::string(v.choice == 1 ? f + " chose a new save together. " : f + " chose to continue your save together. ") +
                        (v.we_ready ? "You are ready." : "Ready up when you are.");
    }
    return v;
}

void lobby_new_save() {
    std::string why;
    if (!worlds_lobby_fresh(&why)) {
        log::warn("Co-op lobby: no fresh co-op save: {}", why);
        return;
    }
    std::lock_guard lock(g_m);
    g_partner_ready = false;
    send({kMsgFresh});
    log::info("Co-op lobby: the host chose a new save together");
}

void lobby_continue() {
    worlds_lobby_continue();
    std::lock_guard lock(g_m);
    g_partner_ready = false;
    log::info("Co-op lobby: the host chose to continue the co-op save");
}

void lobby_toggle_ready() {
    std::lock_guard lock(g_m);
    if (!g_in.connected || g_in.host) return;
    g_we_ready = !g_we_ready;
    send({kMsgReady, static_cast<uint8_t>(g_we_ready ? 1 : 0)});
    log::info("Co-op lobby: {}", g_we_ready ? "ready" : "not ready");
}

void lobby_start() {
    const WorldsLobbyInfo w = worlds_lobby_info();
    std::lock_guard lock(g_m);
    if (!g_in.host || !g_in.connected || !g_partner_ready || g_go_at) return;
    const bool new_game = w.new_game_pending || w.choice == save_io::WorldStart::NewGame;
    g_go_new = new_game;
    g_go_at = now_ms() + kCountdownMs;
    const uint16_t ms = static_cast<uint16_t>(kCountdownMs);
    std::vector<uint8_t> m{kMsgGo, static_cast<uint8_t>(new_game ? 1 : 0), 0, 0};
    std::memcpy(m.data() + 2, &ms, 2);
    send(std::move(m));
    log::info("Co-op lobby: start in {} ms ({})", ms, new_game ? "new game" : "play");
}

void lobby_dismiss() {
    std::lock_guard lock(g_m);
    g_dismissed = true;
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
            case kMsgReady:
                if (b.size() >= 2 && g_in.host) {
                    g_partner_ready = b[1] != 0;
                    log::info("Co-op lobby: {} is {}", g_in.partner_name, g_partner_ready ? "ready" : "not ready");
                }
                break;
            case kMsgGo:
                if (b.size() >= 4 && !g_in.host) {
                    uint16_t ms = 0;
                    std::memcpy(&ms, b.data() + 2, 2);
                    g_go_new = b[1] != 0;
                    g_go_at = now_ms() + std::min<uint16_t>(ms, 10000);
                    log::info("Co-op lobby: the host starts in {} ms", ms);
                }
                break;
            case kMsgFresh:
                fresh = !g_in.host;
                g_we_ready = false;
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
    if (!g_in.connected || own == kUnknown || g_partner_diff == kUnknown || own == g_partner_diff) return {};
    return std::format("Choose the same difficulty\nYou play on difficulty {} and {} on difficulty {}. Pick the same one "
                       "in the game's settings so the fights are as hard for both of you.",
                       own + 1, g_in.partner_name.empty() ? "your friend" : g_in.partner_name, g_partner_diff + 1);
}

}  // namespace cdmp::net
