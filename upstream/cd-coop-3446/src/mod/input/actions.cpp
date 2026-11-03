#include "mod/input/actions.h"

#include <windows.h>

#include "mod/input/pad_input.h"

namespace cdmp::input {
namespace {

constexpr Binding kBindings[] = {
    {"Invite", VK_F1, true, false, kPadY, kPadY, true},
    {"Leave", VK_F2, true, false, kPadB, kPadB, true},
    {"Ping", VK_F3, true, false, kPadLeft, kPadLeft, true},
    {"GoToFriend", VK_F4, true, false, kPadRight, kPadRight, true},
    {"Challenge", VK_F5, false, true, kPadUp, kPadUp, true},
    {"Decline", VK_F6, false, true, kPadDown, kPadDown, true},
    {"Panel", VK_F8, false, false, kPadX, kPadX, true},
    {"Nameplates", VK_F8, true, false, kPadA, kPadA, true},
    {"ReviveNow", 'Q', false, false, kPadX, kPadX, false},
    {"ReviveCheckpoint", 'U', false, false, kPadY, kPadY, false},
    {"RpsScissors", 'U', false, false, kPadX, kPadX, false},
    {"RpsPaper", 'F', false, false, kPadY, kPadY, false},
    {"RpsRock", 'E', false, false, kPadB, kPadB, false},
    {"Skip", VK_ESCAPE, false, false, kPadStart | kPadB, kPadStart, false},
    {"Mash", 'E', false, false, kPadFace, kPadA, false},
    {"MenuPrev", VK_LEFT, false, false, kPadLeft, kPadLeft, false},
    {"MenuNext", VK_RIGHT, false, false, kPadRight, kPadRight, false},
    {"Resync", VK_F8, false, true, kPadRB, kPadRB, true},
};
static_assert(std::size(kBindings) == static_cast<size_t>(Action::Count));

bool key(int vk) { return (GetAsyncKeyState(vk) & 0x8000) != 0; }

}  // namespace

const Binding& binding(Action a) { return kBindings[static_cast<size_t>(a)]; }

bool action_down(Action a, bool focused) {
    const Binding& b = binding(a);
    if (focused && b.vk && key(b.vk) && key(VK_CONTROL) == b.ctrl && (key(VK_SHIFT) == b.shift || (!b.shift && !b.ctrl))) {
        return true;
    }
    const uint16_t pad = pad_buttons();
    if (!(pad & b.pad)) return false;
    return b.chord ? chord_held() : !chord_held();
}

Action action_by_name(std::string_view name) {
    for (size_t i = 0; i < std::size(kBindings); ++i) {
        if (kBindings[i].name == name) return static_cast<Action>(i);
    }
    return Action::Count;
}

}  // namespace cdmp::input
