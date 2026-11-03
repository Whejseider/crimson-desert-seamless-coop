#pragma once
// Co-op+: the guest's weather follows the host's (user 2026-11-02: "weather HAS to match"), EventKind::Weather.
//
// Weather is not computed from the time of day. The local server picks weather STAGES (stage names Weather_Sunny,
// Weather_Cloudy_65, Weather_Windy_Degree90, Weather_Windy_Speed05, Weather_Rainy_Hard ...) at random when the world
// loads and now and then later, and starts each in the client with PrepareSequencerAck
//   u8 4 | u32 player | u32 player | u64 sequencer | u16 1 | u32 stage key | u32 0 | u32 hash | u32 stage key |
//   f32 x, y, z (where we stood) | ...                                     (103 bytes for a weather stage on 3479)
// and ends one with DeleteSequencerAck (u32 player | u64 sequencer | u32 stage key | u8 reason). Same save, same
// game time, nine loads: nine different sets (packet captures 2026-11-02). So:
// - the host sends its current weather stages (their whole PrepareSequencerAck payloads) on every change and every
//   few seconds;
// - the guest holds back its own server's weather stages (dropped in the sniffer's S>C hook, the ones already
//   running ended once), starts the host's under sequencer ids of its own (kFakeIdBase + n) and ends them when the
//   host's end. After a loading screen they are started again. When the guest leaves (or the host's reports stop),
//   the host's are ended and the guest's own weather stages come back.

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace cdmp::net {

// Game network thread (troc_sniffer's OnRecvPacket hook), real server packets only: records our server's weather
// stages. True = drop this packet (we follow the host's weather).
bool weather_filter_s2c(const uint8_t* packet, size_t size);

// Sniffer thread (capture_observe_c2s): our client's branch and chart requests for the host's stages it plays are
// answered here as the server answers them (our server never heard of those sequencers).
void weather_observe_c2s(const uint8_t* packet, size_t size);

// A Weather event from the other player.
void weather_on_event(const std::vector<uint8_t>& payload);

// Network loop, every frame. `me`: our position (for the stages we start). `in_world`: our position is readable
// (needed to start following; a minigame or cutscene hides it for a while without stopping it).
void weather_tick(bool connected, bool host, bool in_world, std::optional<std::array<float, 3>> me, uint32_t now_ms);

// [coop] weather_sync (default on).
void weather_set_enabled(bool on);

// [dev] weather_test=<stage key>: one-PC test: act as a guest whose host has exactly that weather stage.
void weather_set_test(uint32_t stage_key);

// For the panel / log: the stage name of a weather key ("Rainy_Hard"), "" when not a weather stage.
const char* weather_stage_name(uint32_t stage_key);

// Co-op+ (T08 state audit): the host's weather stages running in our client now.
size_t weather_host_stages_running();

}  // namespace cdmp::net
