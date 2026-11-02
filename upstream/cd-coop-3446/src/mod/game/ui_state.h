#pragma once
// Co-op+: is the game showing a full-screen menu (map, inventory, journal, pause...)?

#include <optional>

namespace cdmp::game {

// True while the game's "gameplay input" flag is off. False when not known (the code that
// owns the flag did not match this build), so callers keep drawing as before.
bool ui_menu_open();

// Co-op+: true while a cutscene (sequencer) plays; nullopt when the flag cannot be read.
std::optional<bool> cutscene_playing();

// Co-op+: address of the minigame strength gauge (float, 0..100), 0 when it cannot be found.
uintptr_t minigame_gauge_address();

}  // namespace cdmp::game
