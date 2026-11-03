#pragma once
// Co-op+: makes the copies we load (the other player's character, enemy copies) visible.
//
// On 3460 a copy finishes loading with a "hide" bit left in its status component (bit 20, the
// game's own loading bits otherwise), so the game draws it with alpha 0: it moves and fights,
// unseen. Once a copy has loaded, this clears those bits through the game's own flag-change
// handler, which sets the alpha back to 1 and applies it. It runs on the game thread, from the
// copy's own per-frame movement step.

#include <cstdint>

namespace cdmp::net {

// Hooks the movement step; false (and a WRN) if the game build does not match.
bool puppet_visibility_install();

// Co-op+ (F9): draws this actor of our own world hidden (0 = none; the previous one is shown again, if it still
// exists). Used for the NPC opponent whose seat the other player's copy takes. Any thread.
void puppet_hide_actor(uint32_t key);
// Every tick while an actor is hidden: the game clears the bits after a while; sets them again.
void puppet_hide_tick();
// Co-op+ (T08 state audit): the actor hidden now (0 = none).
uint32_t puppet_hidden_key();

}  // namespace cdmp::net
