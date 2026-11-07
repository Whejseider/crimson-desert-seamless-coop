#pragma once
// Co-op+ shared XP (user 2026-11-07: "all XP shared between both players"). Crimson Desert's kill experience is
// sub-level experience: KillActor fills a skill-point bar (sub-level 521 Kliff, 522 Oongka, 523 Damiane; 10 / 15 / 25
// by tier, 140 elite, 1000 boss). Each player's server is in their own game, so a kill in one world never fills the
// other player's bar. Here every skill-point gain our server reports (AddSubLevelExperienceAck) goes to the partner,
// and the partner's mod has ITS server add the same amount through the game's own experience timer handler
// (quest_advance_grant_xp; found with Ghidra, proven in the sandbox gx2: the same AddSubLevelExperienceAck as a kill).
//   - No echo: a gain that our own grant caused is never sent back.
//   - Only while linked (not during the solo prologue, prologue_hold.h); XP earned alone stays one's own.
//   - Only the skill-point sub-levels (521-523); faction contribution and the like stay per player.
//   - RelKind::World type 8: u32 sub-level | u64 amount. Works for any number of players (each gain to each partner).
// [coop] shared_xp (default 1).

#include <cstddef>
#include <cstdint>
#include <vector>

namespace cdmp::net {

inline constexpr uint8_t kMsgSharedXp = 8;  // RelKind::World message type (coop_worlds.cpp routes it here)

void xp_share_configure(bool on);
bool xp_share_enabled();
// Sniffer thread (capture_observe_s2c): our server's messages.
void xp_share_observe_s2c(const uint8_t* packet, size_t size);
// Network loop: the partner's gains granted here (only while `linked`).
void xp_share_tick(bool linked);
std::vector<std::vector<uint8_t>> xp_share_take_outgoing();
void xp_share_on_message(const std::vector<uint8_t>& bytes);

}  // namespace cdmp::net
