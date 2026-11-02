#pragma once
// Co-op+: shared activities (cutscenes and minigames), EventKind::Activity.
//
// Cutscenes run inside each player's own client: the server only sees side effects, there is no "start" or "skip"
// message (captures of the Hernand pub arm wrestling intro, 2026-11-02). We read the game's "cutscene playing" flag
// (game::cutscene_playing) and tell the other player when ours starts and ends. Skipping needs everyone: while the
// other player is in a cutscene too, within kNearM of us, a keyboard hook holds back our Esc and only tells them we
// pressed it; when both have pressed, each mod gives its own game the Esc (D32 as revised by the user 2026-11-02).
// If the other player is not in a cutscene near us, Esc works as in the game.
//
// Minigames: the client asks the server to start one (ActivateMiniGameReq / JoinMiniGameReq), the server spawns the
// NPC opponent (SpawnMiniGameCharacterReq/Ack), the match itself is simulated in the client, and the end is
// LeaveMiniGameReq (names the minigame's stage) + MiniGameResultReq. We tell the other player "started" and
// "finished <game>"; the result bytes did not differ between a win and a loss, so no winner is claimed.
//
// Payload: u32 sequence | u8 type | data. Sent three times on the lossy channel; the receiver drops repeats.
//   type 1 cutscene started | f32 x, y, z     type 2 cutscene ended     type 3 skip pressed | f32 x, y, z
//   type 4 minigame started | u16 minigame id  type 5 minigame finished | u32 stage key
//   type 6 versus (10 Hz in a match) | u8 in a match | u32 E presses this match | f32 gauge (the host's)
// Versus arm wrestling (D35): when both players are in a match (each at their own table), the host owns one gauge
// that both players' E presses push; each mod writes it into its game (game::minigame_gauge_address), so the one who
// reaches 100 wins on both screens.

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
ActivityTick activity_tick(bool connected, bool host, bool focused, std::optional<std::array<float, 3>> me,
                           std::optional<std::array<float, 3>> partner, const std::string& partner_name,
                           uint32_t now_ms);

// [dev] skip_test: one-PC test of the shared skip: during each of our cutscenes act as if the other player were in it
// too and pressed skip 6 s in (the echo peer cannot press keys).
void activity_set_skip_test(bool on);

// [dev] versus_test: one-PC test of versus arm wrestling: a simulated other player in every match of ours, pushing
// this many times a second (0 = off).
void activity_set_versus_test(int presses_per_s);

// Human name of a minigame stage key ("arm wrestling"...), "a minigame" when unknown.
std::string activity_minigame_name(uint32_t stage_key);

}  // namespace cdmp::net
