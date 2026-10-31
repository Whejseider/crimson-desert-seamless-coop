#pragma once
// Party-size enemy toughness in the host's world (Co-op+; rules and reasoning in
// common/party_scaling.h). EXPERIMENTAL and off by default ([scaling] enabled=0):
// the give-back uses VaryStatCheatReq with a positive delta, which no live test
// has exercised yet (guest_hits only ever sends negative ones).
//
// Threads: scaling_observe_hp runs on the sniffer thread (local_capture's
// VaryHPAck); everything else on the network thread.

#include <cstdint>
#include <string>

#include "common/party_scaling.h"

namespace cdmp::net {

void scaling_configure(const scaling::Settings& s);
// Network thread, every loop: how many players share the host's world (1 alone,
// 2 with a guest connected) and whether this PC is the host.
void scaling_set_party(int players, bool host);

// k for the current party (1 when off, alone, or not the host).
float scaling_multiplier();

// guest_hits (host side): the guest's blow after scaling, and a note of the HP
// loss the mod is about to inject so the give-back skips it.
int32_t scaling_guest_damage(int32_t damage);
void scaling_note_injected(uint32_t enemy, int32_t loss_hp);

// local_capture (sniffer thread): an enemy's HP changed. `by_us` = the source of
// the change is our own player (native blows and our injected ones alike).
void scaling_observe_hp(uint32_t enemy, int64_t before_x1000, int64_t after_x1000, bool by_us);

// Network thread: sends the give-backs that are due.
void scaling_tick();

// Panel line ("" when off or alone) and a log summary.
std::string scaling_hud_text();
std::string scaling_describe();

}  // namespace cdmp::net
