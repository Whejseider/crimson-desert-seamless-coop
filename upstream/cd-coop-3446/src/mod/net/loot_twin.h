#pragma once
// Loot for the guest from the host's enemies (docs/research/botin-invitado.md).
//
// A dead copy of the host's enemy exists only in the guest's client: its loot
// prompt gave nothing. When the guest took part in the kill, the guest's own
// server makes a "twin" of that enemy (SpawnCharacterCheatReq, spawn reason
// Cheat) at the copy's place, the twin's load never reaches the guest's
// client, and KillCheatReq kills it at once (option 0: no XP, no quest
// credit, the variant's loot rolled natively). The guest then loots "the
// copy": its interaction requests are sent to the twin, and the twin's loot
// messages come back named after the copy. The host sends no items, only
// "copy N died"; the guest decides with its own data (it hit that copy in
// the last 30 s, is within 30 m of it, a plain enemy variant, rate limits).

#include <cstddef>
#include <cstdint>
#include <vector>

namespace cdmp::net {

// [coop] guest_loot.
void loot_set_enabled(bool on);
// Co-op+ (shared XP, user 2026-11-06: "XP shared within 400 m"): when the host kills an enemy whose copy is within
// 400 m of us, one of our own hidden enemies of the same kind (the loot twin's way) dies by OUR blow -- HP to 0, then
// a collision of our player -- so our server counts a normal kill and gives the experience. No loot unless we also
// earned it (hit it lately, within 30 m). [coop] shared_xp.
void loot_set_shared_xp(bool on);
// Sniffer thread: our server's experience messages (counted and logged, so the XP can be checked).
void loot_observe_s2c(const uint8_t* packet, size_t size);
// Co-op+ (T10): [coop] loot_mode (0 per_player, 1 host_only, 2 first_come).
void loot_set_mode(int mode);
int loot_mode();
// first_come, guest: copies of the host's enemies we looted (to tell the host); each once.
std::vector<uint32_t> loot_take_looted_copies();
// first_come, guest: the host looted the original of this copy: its body here offers nothing any more.
void loot_partner_looted(uint32_t copy);
// Co-op+ dev ([dev] loot_test_original=1): on one PC, loot from the copy's original when no hidden native fits.
void loot_set_test_original(bool on);

// Sniffer writer thread: our own DoFrameEventReq (blows on copies).
void loot_observe_c2s(const uint8_t* packet, size_t size);

// Network thread: a copy of the other player's enemy died here. True if a
// twin is on its way (the copy keeps its loot prompt); false: no loot here.
bool loot_on_copy_died(uint32_t copy, uint32_t variant, uint64_t delay_ms);
// Network thread, every loop: twins that never came; forgotten copies.
void loot_tick();
// Network thread: the copy was removed.
void loot_forget_copy(uint32_t copy);
// Network thread: a dead copy whose twin is still to be looted (or on its
// way): its removal waits (the host's respawn sweep took them, 7 Oct night).
bool loot_keeps_copy(uint32_t copy);
// Network thread: the other player changed or their copies went (copy keys are
// reused by a restarted host).
void loot_reset();

// The game's network thread, every real server->client packet: true = do not
// deliver it (the twin's load and messages). May rewrite the packet in place.
bool loot_filter_s2c(uint8_t* packet, uint16_t size);
// The game's thread posting a client->server packet: interaction requests on
// a copy go to its twin (rewritten in place).
void loot_rewrite_c2s(uint8_t* packet, uint16_t size);

}  // namespace cdmp::net
