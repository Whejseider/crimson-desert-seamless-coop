#pragma once
// Co-op+ (user 2026-11-06): the co-op main menu at the title screen, before anyone plays. "Invite players", "start
// new save", "continue with existing save", the party's Steam names in the top right; the host starts, the guest
// readies up, and both games load at the same moment (a 3 s countdown on both). It keeps the co-op saves' rules
// (coop_worlds.h): one co-op save per friend, never your own; a fresh one sets the old one aside (kept). Difficulty:
// each game reports the one its player chose (ChangeGameDifficultyReq, captured); different ones bring up a card on
// both: choose the same.
//
// The network loop ticks it and sends its messages (inside RelKind::World, types 2..); the render thread draws it
// (render/coop_lobby_ui.cpp) and calls the actions.

#include <cstdint>
#include <string>
#include <vector>

namespace cdmp::net {

struct LobbyInput {
    uint64_t now_ms = 0;
    bool at_title = false;   // the title screen (no character, not loading, the game window up)
    bool connected = false;
    bool host = false;
    std::string own_name, partner_name;
};
void lobby_tick(const LobbyInput& in);

struct LobbyView {
    bool show = false;            // drawn now (at the title, not dismissed)
    bool at_title = false;        // at the title (shown or dismissed)
    bool host = true;
    bool connected = false;
    bool worlds_on = false;       // co-op saves are on (else co-op uses each player's own save)
    std::string own_name, partner_name;
    bool partner_ready = false;   // host: the guest is ready
    bool we_ready = false;        // guest
    bool can_continue = false;    // a co-op save with this friend that has been played
    int choice = 0;               // 0 not chosen, 1 a new save, 2 continue
    bool save_ready = false;      // our game's saves are the co-op save (switched at the title)
    int countdown = -1;           // seconds left before both games start (-1: none)
    bool difficulty_mismatch = false;
    std::string status;           // one line under the menu
};
LobbyView lobby_view();

// Render thread: the menu's actions.
void lobby_new_save();      // host
void lobby_continue();      // host
void lobby_toggle_ready();  // guest
void lobby_start();         // host (both ready): everyone starts in 3 s
void lobby_dismiss();       // "Play solo" / Esc: the game's own title until the next time we are at the title
void lobby_reopen();        // F8 at the title

// Network loop.
std::vector<std::vector<uint8_t>> lobby_take_outgoing();  // RelKind::World messages
void lobby_on_message(const std::vector<uint8_t>& bytes);
// Capture writer: our client's requests (the difficulty the player chose).
void lobby_observe_c2s(const uint8_t* packet, size_t size);
// The difficulty card (both players chose different ones): "" = none.
std::string lobby_difficulty_card();

}  // namespace cdmp::net
