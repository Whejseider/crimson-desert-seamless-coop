#pragma once
// Co-op+: every action the mod adds, with its keyboard keys and its controller buttons, in one table. Code asks
// action_down(); the overlay draws the action's button for the current input method (render/glyphs.h), so a prompt
// written as "{Challenge} Challenge" shows Shift+F5 on a keyboard, View + D-pad Up on an Xbox pad and Touchpad +
// D-pad Up on a PlayStation pad.
//
// Controller layout. The game uses every button (Game8 controls list, docs/research/progression.md), so the co-op
// actions are chords: hold View (Xbox) / the touchpad button or Create (PlayStation), then press
//   Y/Triangle invite   B/Circle leave   D-pad Left ping   D-pad Right go to friend
//   D-pad Up challenge / accept / end   D-pad Down decline   X/Square panel   A/Cross name tags
// A tap on View still reaches the game (camera). In a Co-op+ box the plain buttons answer it:
//   revive  X/Square instant, Y/Triangle checkpoint     rock-paper-scissors  X scissors, Y paper, B rock
//   cutscene skip  Start/Options or B                   arm wrestling mash  any face button

#include <cstdint>
#include <string_view>

namespace cdmp::input {

enum class Action : uint8_t {
    Invite, Leave, Ping, GoToFriend, Challenge, Decline, Panel, Nameplates,
    ReviveNow, ReviveCheckpoint, RpsScissors, RpsPaper, RpsRock, Skip, Mash,
    Count
};

struct Binding {
    std::string_view name;   // the token in prompt texts: "{Challenge}"
    int vk;                  // keyboard key (0 = none)
    bool ctrl, shift;        // keyboard modifiers
    uint16_t pad;            // pad button(s): any of them
    uint16_t pad_shown;      // the one drawn
    bool chord;              // needs the chord modifier held
};

const Binding& binding(Action a);

// The action's keyboard keys or pad buttons are down now. `focused`: the game window is in front (keyboard only).
bool action_down(Action a, bool focused);

// Rising edge helper: one per caller and action.
class Edge {
public:
    bool operator()(bool down) {
        const bool r = down && !was_;
        was_ = down;
        return r;
    }

private:
    bool was_ = false;
};

// Action by its token name ("Challenge"), Count when unknown.
Action action_by_name(std::string_view name);

}  // namespace cdmp::input
