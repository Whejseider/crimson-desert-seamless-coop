#pragma once
// Co-op+ (3479): our own revive prompt, for when the game's death screen does not come.
//
// Once the other player's copy has been loaded into our world, the game never again opens its
// "Death has found you / Instant Revive / Revive at Checkpoint" screen when OUR player dies, even
// after the copy is gone (one-PC bisect 2026-11-02, NOTES/LOG.md: copy never loaded -> screen;
// copy removed 5 s before the death, with or without its user, with or without any replayed
// event -> no screen). The screen is a data-driven UI script; its condition is not found yet.
//
// Until it is, this offers the same two choices on the overlay and sends what the game sends
// (captured on 3479 from a death without co-op):
//   Q  Instant Revive       ReviveReq (2957): u32 player | 00 01 00 00
//   U  Revive at Checkpoint ReviveLoadingStartReq (3139): 04 02 00; 1 s later ReviveLoadingReq (2390): 02;
//                           then, as the game does when it starts its loading screen, GameLoadingStartReq
//                           (2294) + ReviveReq (type 2). The server revives us at the checkpoint and names it
//                           in LoadingFieldDataAck (+28 position); the client, not in its own loading flow,
//                           ignores that. We send it DoTeleportAck (3052: u32 actor | vec3 | 8 B) to the
//                           checkpoint, at once and up to 2 more times. BEST EFFORT: it took in 1 of 4 live
//                           tests (the one where we were still moving); otherwise we are revived in place
//                           with full health. No loading screen either way.
// Lead for the real fix: the client only accepts ReviveLoadingStartAck of the type it asked for itself
// (status component +0x471, 16 = none; handler exe+0x6F33D0). We set it (the teleport took with it), but
// that alone does not bring the loading screen: the next step still waits for the screen.
// The server decides as it does for its own screen (e.g. no instant revives left).

#include <cstddef>
#include <cstdint>
#include <optional>

namespace cdmp::net {

// What the overlay shows.
enum class RevivePrompt : uint8_t {
    None,
    Choice,          // Q Instant Revive, U Revive at Checkpoint
    WaitBoss,        // A9: died in a boss fight, the other player still fights: wait
    CheckpointOnly,  // A9: everyone is down: the fight starts again from the checkpoint (U only)
};

// A9 ([coop] boss_death_wait, user request 2026-11-01): a player who dies during a boss fight
// (boss_safe.h: a boss whose HP changed in the last 30 s) waits. When the other player beats the
// boss we get up where we fell with full health (the server's checkpoint revive, client not
// moved); when the other player is down too, only the checkpoint revive is offered.
void revive_set_boss_wait(bool on);

// Network loop, every pass. `hp`/`hp_max`: our player (hp_max 0 = unknown). `partner_hp`: the other
// player's health (none when nobody is connected). `focused`: the game window has the keyboard.
RevivePrompt revive_prompt_tick(bool connected, float hp, float hp_max, std::optional<float> partner_hp, bool focused,
                                uint32_t now_ms);

// Sniffer thread (capture_observe_s2c): the checkpoint the server revives us at (LoadingFieldDataAck).
void revive_observe_s2c(const uint8_t* packet, size_t size);

}  // namespace cdmp::net
