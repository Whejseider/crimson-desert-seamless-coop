#pragma once
// Co-op+ (user 2026-11-07, after Borque's and Plitos's reports: the opening tutorial broke when played together:
// "after the first dialog with Oongka neither of us could talk to Marius", the camp's horses could be ridden out,
// "progress broke when one player's cutscene ended earlier"): the PROLOGUE is played SOLO. While either player is
// still in it, the co-op link stays up (chat, quick messages, the party frame, the lobby, notices) but everything that
// reaches into the other player's game waits: their character, shared cutscenes and the skip vote, minigames and
// duels, live quests and the story copy, time and weather, enemies, revive and travel. It works like being far apart.
//
// "In the prologue" is read from the player's own save (the newest one in the save folder in use, co-op saves
// included): fewer than [coop] prologue_missions completed missions (default 60; the real count is logged so it can
// be tuned), or no save at all yet, or a New Co-op Save run from the title menu until its saves say otherwise. Read
// again on every load and every 30 s (the game autosaves as the story moves on). It also ends, for good, the first time
// the player rides anything for 5 s (user 2026-11-07: "automatically rather than the players having a button to
// press... the first time they get on a horse"; "one of the first things you do after the prologue is ride into town
// with an old man", and the prologue has no riding). Each mod tells the other its own state (RelKind::World
// type 7: u8 in prologue | u32 completed missions); co-op waits while either is in it. [coop] solo_prologue=0 turns
// all of it off.

#include <cstdint>
#include <string>
#include <vector>

namespace cdmp::net {

inline constexpr uint8_t kMsgPrologue = 7;  // RelKind::World message type (coop_worlds.cpp routes it here)

void prologue_configure(bool enabled, uint32_t threshold);

struct PrologueInput {
    uint64_t now_ms = 0;
    bool connected = false;
    bool in_world = false;      // our character is loaded
    uint32_t generation = 0;    // capture_template_generation: moves on with every load
    bool new_save_run = false;  // the title menu started a New Co-op Save run
    uint32_t riding = 0;  // capture_carried_for(5 s): what has carried us (a horse, riding along), 0 = nothing
    std::string partner_name;
};
// Network loop, every pass.
void prologue_tick(const PrologueInput& in);

// Co-op waits (either player in the prologue, while connected). Any thread.
bool prologue_held();
// We are in the prologue ourselves.
bool prologue_ours();
// Our prologue is done, whatever the save says (until a new game): the first ride; the dev script.
void prologue_done_now();
// One line for the party frame while co-op waits ("" = none).
std::string prologue_line();

std::vector<std::vector<uint8_t>> prologue_take_outgoing();
void prologue_on_message(const std::vector<uint8_t>& bytes);

// The other player's Activity events that still pass while co-op waits (chat, menu, notices).
bool prologue_activity_allowed(uint8_t type);

}  // namespace cdmp::net
