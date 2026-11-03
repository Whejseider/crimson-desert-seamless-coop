#pragma once
// Co-op+ (roadmap T36): a kill feed. When one of our blows (or arrows) took down an enemy or an animal, both players
// get a line on the right of the screen for 6 s: "You defeated a Deer" here, "<name> defeated a Deer" there (the
// friend's lines in their colour). Our kills are told apart the way the hunting contest does (coop_hunt.h).
// Activity type 23: the victim's name (UTF-8, "" = an enemy whose kind has no name here).

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace cdmp::net {

// Game -> client packets (local_capture), every one.
void kill_feed_observe_s2c(const uint8_t* packet, size_t size);

// Activity type 23 from the other player.
void kill_feed_on_event(const uint8_t* data, size_t n);

struct FeedLine {
    std::string text;
    bool ours = false;
};
// Network loop: sends our kills, returns the lines to show (oldest first).
std::vector<FeedLine> kill_feed_tick(bool connected, const std::string& partner_name, uint32_t now_ms);

}  // namespace cdmp::net
