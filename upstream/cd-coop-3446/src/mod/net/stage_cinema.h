#pragma once
// Co-op+: the host's stage cinematics play for the guest too (user 2026-11-02: "you HAVE to be able to play boss
// fights together (hence needing boss fight cutscenes to play for you)"). D38.
//
// A boss fight, a story scene, a minigame intro: each is a STAGE (a .pastage graph + a .paseq timeline) that the
// player's own client runs; the server starts it (PrepareSequencerAck: the stage key and the world actors bound to its
// nodes) and moves it between branches (StageBranchStateAck / StageOnBranchEventAck); the cinematic is a branch of
// it, played by the client. The guest's world does not run the host's boss stage (its boss only exists in the host's
// world; the guest's own is hidden near the host), so the guest's client never plays the cinematic.
//
// So, when the host's cutscene starts inside a stage (SetAvoidByCutSceneAck names the stage, or the stage that changed
// branch just before), the host sends that stage's PrepareSequencerAck payload (the actors it binds translated to the
// guest's copies of them, enemy_copy_of) with its current branch messages; branch changes follow while the cutscene
// plays. A guest within 100 m starts a copy of the stage in its client under a sequencer id of its own (kFakeIdBase +
// n, its server never hears of it), at the host's branch, answers the client's chart requests for it as the server
// would (weather_sync does the same), and ends the copy when the host's cutscene ends (or after kMaxMs). The skip
// rule of coop_activity (everyone must press Esc) then covers both cutscenes.

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace cdmp::net {

// Game network thread (troc_sniffer's OnRecvPacket hook), real server packets: records stages and their branches.
// Never drops anything.
void cinema_observe_s2c(const uint8_t* packet, size_t size);

// Sniffer thread (capture_observe_c2s): chart requests of our client for a stage copy we started.
void cinema_observe_c2s(const uint8_t* packet, size_t size);

// coop_activity: the other player's cinema events (Activity types 7 start, 8 end, 9 branch).
void cinema_on_event(uint8_t type, const uint8_t* data, size_t size);

// coop_activity's tick (every frame). `cut`: our cutscene flag.
void cinema_tick(bool connected, bool host, bool cut, std::optional<std::array<float, 3>> me,
                 std::optional<std::array<float, 3>> partner, uint32_t now_ms);

// The stage key of one of our server's running stages (0 = unknown), e.g. the minigame a match is in.
uint32_t cinema_stage_key(uint64_t sequencer);

// The minigame stage among `keys` nearest to `pos` within `max_m` (its prepare carries the spot), 0 when none.
uint32_t cinema_minigame_near(const std::array<float, 3>& pos, float max_m, const uint32_t* keys, size_t n);

// [coop] stage_cinema (default on).
void cinema_set_enabled(bool on);

// [dev] cinema_test: one-PC test: 3 s into each of our own stage cutscenes, start a copy of that stage as if the
// host had sent it (shows whether a second copy at the same branch plays, and that ending it cleans up).
void cinema_set_test(bool on);

// Co-op+ (T08 state audit): a copy of the host's cutscene stage is pending or running in our client.
bool cinema_copy_active();

}  // namespace cdmp::net
