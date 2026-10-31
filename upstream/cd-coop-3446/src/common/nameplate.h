#pragma once
// How strongly the other player's nameplate is drawn at a distance (Co-op+,
// brief 6.2: "Steam usernames above heads, distance-faded, toggleable").
// Pure logic, unit-tested in tests/tests.cpp; hud.cpp applies it.

#include <algorithm>

namespace cdmp::nameplate {

inline constexpr float kFullUntilM = 25.0f;  // fully opaque this close
inline constexpr float kFadedAtM = 200.0f;   // faded down to kMinAlpha by here
inline constexpr float kMinAlpha = 0.35f;    // still readable when far
inline constexpr float kHiddenFromM = 600.0f;  // beyond this the map pin does the job

// 0 = do not draw, else the opacity in (0, 1].
inline float alpha(float distance_m) {
    if (!(distance_m >= 0.0f) || distance_m >= kHiddenFromM) return 0.0f;
    if (distance_m <= kFullUntilM) return 1.0f;
    const float t = std::min(1.0f, (distance_m - kFullUntilM) / (kFadedAtM - kFullUntilM));
    return 1.0f - t * (1.0f - kMinAlpha);
}

// Text scale: a little smaller far away, never below 80%.
inline float text_scale(float distance_m) {
    if (!(distance_m > kFullUntilM)) return 1.0f;
    const float t = std::min(1.0f, (distance_m - kFullUntilM) / (kFadedAtM - kFullUntilM));
    return 1.0f - 0.2f * t;
}

}  // namespace cdmp::nameplate
