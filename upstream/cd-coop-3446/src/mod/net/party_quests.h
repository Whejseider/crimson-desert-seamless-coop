#pragma once
// Co-op+ party journal (user 2026-11-02: "party based shared progression ... quests, missions, story shared across the
// party"). D42.
//
// What the game allows: each player's quests live in their own in-process server, which advances them from that
// player's own world (dialogs, triggers, native kills) and ignores progress requests from outside (the quest and
// knowledge cheat requests get no answer on retail, LOG 2026-11-02). So a quest cannot be completed in the guest's
// save by the host's actions. What is shared instead: the journal. Our server lists our missions in
// UpdateMissionListAck (S>C 2360): records of u32 mission key | u8 state (2 under way, 5 done) | u8 kind | ...; when
// one of ours turns done, the other player is told (Activity type 13: u32 key), and they get the game's
// "quest complete" notice with its name (mission_names.inc, from the game data), marked when they have the same
// quest under way: walking the same objective together (same place, same dialog) finishes it in both worlds.

#include <cstddef>
#include <cstdint>
#include <string>

namespace cdmp::net {

// Our server's messages: our mission states.
void party_quests_observe_s2c(const uint8_t* packet, size_t size);

// Activity type 13 from the other player (payload after the Activity header).
void party_quests_on_event(const uint8_t* data, size_t n);

// Once per network loop: our completions to send (through activity_send), and the next notice to show ("" = none),
// "title\nbody" (the overlay's notice format). `who`: the other player's name.
std::string party_quests_tick(bool connected, const std::string& who);

// [dev] quest_test: one-PC test with the echo peer: 60 s after connecting, act as if we completed this mission (its
// echo then shows as the other player's).
void party_quests_set_test(uint32_t key);

// The game's name of a mission ("" when unknown).
std::string party_quests_name(uint32_t key);

// Co-op+ (T39): spoiler guard: the friend's quest notices name only quests we are on or have done.
void party_quests_set_spoiler_guard(bool on);

}  // namespace cdmp::net
