#pragma once
// Co-op+: shared activities (cutscenes and minigames), EventKind::Activity.
//
// Cutscenes run inside each player's own client: the server only sees side effects, there is no "start" or "skip"
// message (captures of the Hernand pub arm wrestling intro, 2026-11-02). We read the game's "cutscene playing" flag
// (game::cutscene_playing) and tell the other player when ours starts and ends. Skipping is shared: Esc during our
// cutscene tells the other player, and if they are in a cutscene too, within kNearM of us, their mod presses Esc for
// them ("anyone's skip button skips for everyone", D32).
//
// Minigames: the client asks the server to start one (ActivateMiniGameReq / JoinMiniGameReq), the server spawns the
// NPC opponent (SpawnMiniGameCharacterReq/Ack), the match itself is simulated in the client, and the end is
// LeaveMiniGameReq (names the minigame's stage) + MiniGameResultReq. We tell the other player "started" and
// "finished <game>"; the result bytes did not differ between a win and a loss, so no winner is claimed.
//
// Payload: u32 sequence | u8 type | data. Sent three times on the lossy channel; the receiver drops repeats.
//   type 1 cutscene started | f32 x, y, z     type 2 cutscene ended     type 3 skip pressed | f32 x, y, z
//   type 4 minigame started | u16 minigame id  type 5 minigame finished | u32 stage key

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace cdmp::net {

// Client requests we watch (minigame start/end).
void activity_observe_c2s(const uint8_t* packet, size_t size);

// An Activity event from the other player.
void activity_on_event(const std::vector<uint8_t>& payload);

struct ActivityTick {
    std::string banner;  // persistent line while the other player is in a cutscene ("" = none)
    std::string toast;   // one-off message ("" = none)
};

// Network loop, every frame: reads our cutscene flag, our Esc key, and applies the other player's skips.
// `me` / `partner`: positions (partner unset = not connected). `focused`: the game window is in front.
ActivityTick activity_tick(bool connected, bool focused, std::optional<std::array<float, 3>> me,
                           std::optional<std::array<float, 3>> partner, const std::string& partner_name,
                           uint32_t now_ms);

// [dev] skip_test: one-PC test of the shared skip: 3 s into each of our cutscenes act as if the other player pressed
// skip (the echo peer cannot press keys).
void activity_set_skip_test(bool on);

// Human name of a minigame stage key ("arm wrestling"...), "a minigame" when unknown.
std::string activity_minigame_name(uint32_t stage_key);

}  // namespace cdmp::net
