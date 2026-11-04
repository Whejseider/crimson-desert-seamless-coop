#pragma once
// Co-op+ co-op worlds (user 2026-11-04, "like Elden Ring Seamless Co-op; protect every player's saves"):
//   - your own (solo) saves are only ever used when you play alone;
//   - every friend you play with gets a co-op world of their own, for the host AND the guest: a separate set of save
//     slots (save_io.h), made the first time as a copy of your own slots;
//   - when a co-op session starts, both games switch into their world with that friend; when it ends (the friend
//     stays away), both switch back to their own saves.
// A switch is the game's own way of changing saves: Esc > Others > Title Screen > confirm, the save folder is changed
// while the game sits at the title (nothing is being saved then), then Play (E), which loads the newest save of the
// world, i.e. where you left off with that friend. A countdown notice comes first; keys and clicks are only sent while
// the game window is in front; every step has a timeout and says what went wrong.

#include <cstdint>
#include <string>

namespace cdmp::net {

// At start. enabled: [coop] coop_worlds. test_partner: [dev] coop_world_partner, a partner key used for one-PC tests
// (the echo peer has no Steam id).
void worlds_configure(bool enabled, const std::string& test_partner);

struct WorldsInput {
    uint64_t now_ms = 0;
    bool connected = false;
    std::string partner_key;   // "steam-<id64>" (or the test key); "" when unknown
    std::string partner_name;
    bool in_world = false;     // our character is loaded (not the title screen, not a loading screen)
    bool menu_open = false;    // a full-screen menu (map, inventory, pause...)
    bool host = false;         // we host the session (the guest's world follows the host's story)
};
// Network loop, every tick.
void worlds_tick(const WorldsInput& in);

std::string worlds_hud_text();    // panel line ("" = nothing to say)
std::string worlds_take_toast();  // a one-off notice
std::wstring worlds_active();     // the world the game's saves come from now ("" = your own saves)

}  // namespace cdmp::net
