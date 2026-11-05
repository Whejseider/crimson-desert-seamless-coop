#pragma once
// Co-op+ (2026-11-05, roadmap T41 "hide and seek"): a challenge game ("Hide and seek" in the picker).
//
// Each round one player counts and the other hides. The one who sent the challenge counts first; the roles swap
// every round. The seeker owns the round: it sends the start with its own position, the centre of the hiding area
// (kAreaM around it). For kHideMs the seeker's screen is veiled (eyes closed, counting) while the hider runs off; then
// the seeker has kSeekMs to come within kFoundM of the hider. During the round the hider is hidden from the seeker:
// no name tag, no distance on the partner frame, no minimap icon (the copy is loaded again without it) and no map pin.
// The seeker only gets warmer / colder from the distance. A hider who stays outside the area for kOutMs gives the
// round away. First to 2 rounds (coop_activity.cpp).
//
// Activity type 26: u8 op | u32 round | data.  op 1 start | f32 x, y, z (the area's centre)   op 2 found | u32 ms
// op 3 not found (time ran out)   op 4 the hider left the area.

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>

namespace cdmp::net {

inline constexpr uint32_t kHideStage = 0xD0E10004;  // the challenge "stage" of hide and seek

// Activity type 26 from the other player.
void hideseek_on_event(const uint8_t* data, size_t n);

struct HideSeekTick {
    std::string banner;
    std::string toast;
    int round_result = 0;  // this tick a round ended: 1 we won, 2 they did
};
// `inviter`: we sent the challenge (we count first).
HideSeekTick hideseek_tick(bool on, bool connected, bool inviter, const std::string& who,
                           std::optional<std::array<float, 3>> me, std::optional<std::array<float, 3>> partner);

struct HideSeekHud {
    std::string title, subtitle;  // the big title card ("" = none)
    unsigned int color = 0;
    float alpha = 0;
    bool veil = false;            // the seeker's eyes are closed: the screen is veiled
    std::string veil_count, veil_line;
    bool area = false;            // the hiding area, drawn as a ring on the ground
    std::array<float, 3> area_centre{};
    float area_radius = 0;
    bool outside = false;         // we hide and stand outside it (the ring turns red)
};
HideSeekHud hideseek_hud();

// We are seeking: the friend is hidden from us (name tag, frame distance, minimap icon, map pin).
bool hideseek_partner_hidden();
// A hide-and-seek challenge is running (no "go to your friend" now).
bool hideseek_active();

// [dev] hideseek_test: one-PC tests. 1 = we seek, a simulated hider stands 36 m away (the echo copy is ignored);
// 2 = we hide (the simulated seeker sends the start and never finds us). Short phases (20 s hiding, 45 s seeking).
void hideseek_set_test(int mode);

}  // namespace cdmp::net
