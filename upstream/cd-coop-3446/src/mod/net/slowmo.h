#pragma once
// Co-op+: no slow motion in co-op (playtester poll, 2026-11-04).
//
// Focus slows the world through a "special mode" the game's in-process server switches on for the player
// (EnableSpecialModeAck, mode 101) and off again (DisableSpecialModeAck). Each game runs its own world, so one
// player's slow motion would leave the two worlds at different speeds. While a co-op session is up that switch-on is
// not passed to the client. Alone, nothing changes.

#include <cstdint>

namespace cdmp::net {

// block: [coop] block_slowmo (in co-op sessions); always: [dev] block_slowmo_always (one-PC tests, no partner needed).
void slowmo_configure(bool block, bool always);
// The sniffer's receive hook: true = drop this server packet (Focus's EnableSpecialModeAck while blocking).
bool slowmo_filter_s2c(const uint8_t* packet, uint16_t size);
void slowmo_set_connected(bool connected);
// F8 setting, live.
void slowmo_set_block(bool block);
bool slowmo_blocking();

}  // namespace cdmp::net
