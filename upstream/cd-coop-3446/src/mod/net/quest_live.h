#pragma once
// Co-op+ live quests: a quest step one player finishes finishes for the other player too, as it happens (no reload).
//
// Each player's quests run in their own game's in-process server, and a step there finishes only through requests
// from that player's own client (the retail server drops the quest cheat: CompleteQuestByCheatReq is parsed and
// discarded, 2026-11-04). So the mod copies the client requests that finish steps from one player's client to the
// other player's server:
//   MissionTriggerInOutReq (2899: u32 mission, u32 user key, u8 in/out): "go to / reach / enter" steps. The receiver
//     writes its own user key (UpdateMissionListAck 2360 carries it at offset 6). Proven 2026-11-04: on a fresh load,
//     nobody moving, one such request finished "Head to Hernand" and the server moved the quest to its next step.
//   QuestDialogEndReq (2223, 30 bytes: u32 NPC actor, u32 dialog, u32 x3, u8 x2, u32 x2): "talk to" steps. The NPC's
//     key differs per world, so the receiver uses its own NPC that offers the same dialog (SetQuestDialogAck 3086
//     lists each nearby NPC's dialogs). Not proven yet (no recording of a talk step).
// The server checks every request as it would its own player's (the step must be under way, in order; a step of
// another kind, or not started yet, was refused in the tests), so nothing can skip ahead or break a quest.
// Only while BOTH players are in a co-op world (coop_worlds.h): never the players' own saves.
//
// Wire (RelKind::Quest): u8 version 1 | u8 flags (1 = the sender is in a co-op world) | u16 opcode | payload.

#include <cstdint>
#include <string>
#include <vector>

namespace cdmp::net {

// At start. enabled: [coop] live_quests. any_world: [dev] live_quests_any_world (one-PC tests: also outside a
// co-op world; never for players).
void quest_live_configure(bool enabled, bool any_world);

// The sniffer's copies of the game's packets (any thread).
void quest_live_observe_c2s(const uint8_t* packet, size_t size);
void quest_live_observe_s2c(const uint8_t* packet, size_t size);

// Network loop.
void quest_live_tick(bool connected, const std::string& partner_name, uint64_t now_ms);
void quest_live_on_message(const std::vector<uint8_t>& bytes);
// [coop] live_quests (set by quest_live_configure).
bool quest_live_enabled();
std::vector<std::vector<uint8_t>> quest_live_take_outgoing();

std::string quest_live_take_toast();  // "" = none
std::string quest_live_hud_text();    // "" = nothing to say

}  // namespace cdmp::net
