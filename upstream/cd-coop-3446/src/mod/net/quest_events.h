#pragma once

#include <cstdint>
// Co-op+ dev diagnostic ([dev] quest_events_log=1): logs the game events the quest system is fed.
//
// MissionTriggerInOutReq's handler (exe+0x2EEE800) does not finish a step itself: it builds a
// pa::GameEventHandlerParameter (vtable exe+0x58FED20; +0x08 u8 type, 0x11 trigger in / 0x12 out,
// +0x10 u32 mission, +0x14 u32 user) and posts it with exe+0x19434D0(handler, param). About 300
// places build these events (kills, dialogs, interactions...). This logs every post (type, caller,
// the first 0x70 bytes) so a test shows which event a kill or a talk produces.

namespace cdmp::net {

void quest_events_enable(bool on);
// Late init; no-op unless enabled.
void quest_events_install();
// Dev experiment: ask the quest server to advance the mission with this key (exe+0x2B95BE0, as when the game sees a
// step's condition met), through quest_advance.h WITHOUT its "under way here" check. Needs the live quest hooks.
void quest_events_request_advance(uint32_t mission_key);

}  // namespace cdmp::net
