#pragma once
// Co-op+ (roadmap T30, T40): the co-op settings in the F8 panel.
//
// While the panel is open, Up / Down (arrow keys, or the D-pad with no chord) pick a setting and Left / Right change
// it. A change is written to cdmp.ini at once (WritePrivateProfileString keeps the file's UTF-16) and, where the mod
// can, applied live; the others say "(next start)". Party scaling multipliers (T40) are here too: they are stored as
// [scaling] panel_health_2p / panel_boss_health_2p, which win over scaling.toml's 2-player values (0 = the file's).
//
// Hotkeys are rebindable under [keys] in cdmp.ini (input/actions.h: load_bindings); the panel lists them.

#include <string>
#include <vector>

#include "mod/config.h"

namespace cdmp::settings {

// At start, after the config is read. Live values start from it.
void init(const std::wstring& ini_path, const Config& cfg, float health_2p, float boss_health_2p);

// The panel is drawn (render/hud.cpp tells us every frame; the input code asks).
void set_panel_open(bool open);
bool panel_open();

// Keyboard hook thread: true = the key is ours (an arrow while the panel is open).
bool hook_key(unsigned vk, bool down);
// Pad code: D-pad steps while the panel is open.
void step(int dx, int dy);

struct Line {
    std::string text;
    bool selected = false;
};
// The settings block of the panel.
std::vector<Line> lines();

// Live values the network loop reads (the config is const there).
bool respawn_at_partner();
bool net_hud();  // T32: the small ping / loss readout
bool colourblind();  // T34: the colour-blind-safe player colours
float duel_ring_m();  // T42: the duel ground's radius when we place it

}  // namespace cdmp::settings
