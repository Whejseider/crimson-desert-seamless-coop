#pragma once
// Co-op+ (2026-11-05, roadmap T41 "fishing contest"): a challenge game ("Fishing contest" in the picker).
//
// After 3, 2, 1 each player fishes in their own world for kRoundMs; every fish they keep scores by its kind: river
// fish 1, sea fish 2, legendary fish 5. A kept fish is the server's AddItemToInventoryAck with the catch reason
// (0x0D0F) and a fish item key (recorded 2026-11-05: Bass 29816 after "Keep"); a released fish scores nothing.
// Scores go to the other player as they change (Activity type 27: u32 round | u16 score | u16 fish | u32 last
// fish item key), so each side can say what the other caught. Highest score when time runs out wins; a draw plays
// another round.

#include <cstddef>
#include <cstdint>
#include <string>

namespace cdmp::net {

inline constexpr uint32_t kFishStage = 0xD0E10005;  // the challenge "stage" of a fishing contest

// Our server's messages: our catches.
void fishing_observe_s2c(const uint8_t* packet, size_t size);

// Activity type 27 from the other player.
void fishing_on_event(const uint8_t* data, size_t n);

struct FishingTick {
    std::string banner;
    std::string toast;
    int round_result = 0;  // this tick a round ended: 1 we won, 2 they did, 3 a draw
};
FishingTick fishing_tick(bool on, bool connected, const std::string& who);

struct FishingHud {
    std::string title, subtitle;
    unsigned int color = 0;
    float alpha = 0;
};
FishingHud fishing_hud();

// The display name of a fish item ("Bass"), nullptr when the item is not a fish.
const char* fishing_fish_name(uint32_t item_key);

// [dev] fishing_test: one-PC test: the echo of our own score is ignored (the partner stays at 0).
void fishing_set_test(bool on);

}  // namespace cdmp::net
