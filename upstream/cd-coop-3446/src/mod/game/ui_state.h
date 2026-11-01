#pragma once
// Co-op+: is the game showing a full-screen menu (map, inventory, journal, pause...)?

namespace cdmp::game {

// True while the game's "gameplay input" flag is off. False when not known (the code that
// owns the flag did not match this build), so callers keep drawing as before.
bool ui_menu_open();

}  // namespace cdmp::game
