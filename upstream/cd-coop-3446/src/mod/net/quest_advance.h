#pragma once
// Co-op+ live quests: finish a quest step through the game's own quest advancement routine.
//
// Found 2026-11-04 (memory diff + hardware watch, docs/design/coop-worlds.md §4): when a step's condition is met the
// in-process quest server calls exe+0x2B95BE0(manager, u32* result, u16* mission index, u64* time, u8 0). It marks
// the step done, starts the next one and runs the rest (rewards, quest gauge, a cleared camp's spawns removed) - the
// same as playing it. Proven on one PC for go-to, defeat and talk steps. It also starts AND finishes a step that was
// not started, so callers must only ask for a step that is under way here (quest_live checks).
//
// It must run on the quest server's thread: a request waits until the server handles the next
// MissionTriggerInOutReq, and quest_advance_request sends a harmless one (mission 0) to wake it.

#include <cstdint>

namespace cdmp::net {

// Hooks the server's mission state setter (to learn the quest manager) and MissionTriggerInOutReq's handler (where
// requests run). Build 1.0.0.3527 only (each site is checked); false if a site does not match.
bool quest_advance_install();
// True once the quest manager is known (after a save has loaded).
bool quest_advance_ready();
// Queue the step with this mission key; dropped if the server has not run it within 10 s.
void quest_advance_request(uint32_t mission_key);

}  // namespace cdmp::net
