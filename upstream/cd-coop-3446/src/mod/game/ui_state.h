#pragma once
// Co-op+: is the game showing a full-screen menu (map, inventory, journal, pause...)?

#include <optional>

namespace cdmp::game {

// True while the game's "gameplay input" flag is off. False when not known (the code that
// owns the flag did not match this build), so callers keep drawing as before.
bool ui_menu_open();

// Co-op+: true while a cutscene (sequencer) plays; nullopt when the flag cannot be read.
std::optional<bool> cutscene_playing();

}  // namespace cdmp::game
