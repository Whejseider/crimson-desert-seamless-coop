#pragma once
// Co-op+ (2026-11-05, user: "workshop features"): the session summary. When two friends part ways, a card sums up
// the time they spent together: how long, the enemies each brought down, challenges won and lost, revives, quest
// steps finished together, bosses and the distance travelled side by side (the game's own results-screen look).
//
// A session runs from connecting until the friend has been gone kGoneMs without coming back (a resync or a dropped
// connection that reconnects by itself carries on the same session). Short sessions (under kMinMs) show no card. The
// card waits while another co-op card is up, a menu is open or we are not in the world (a co-op world switch shows
// it once we are back in our own save), then stays kShowMs.

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace cdmp::net {

enum class SummaryStat { OurKill, TheirKill, MatchWon, MatchLost, WeRevived, TheyRevivedUs, QuestStep, Boss, Count };

// Any module, any thread: something happened while connected (ignored when no session runs).
void summary_note(SummaryStat stat);

// Network loop, every frame. `me` / `partner`: positions (unset = unknown).
void summary_tick(bool connected, const std::string& partner_name, std::optional<std::array<float, 3>> me,
                  std::optional<std::array<float, 3>> partner, uint32_t now_ms);

struct SummaryRow {
    std::string label, ours, theirs;  // theirs "" = one value across both columns
};
struct SummaryCard {
    bool pending = false;  // a card waits to be shown
    std::string title, subtitle, partner;
    std::vector<SummaryRow> rows;
};
// The card waiting to be shown (pending = false: none).
SummaryCard summary_card();
// The overlay drew it for the first time (its timer starts) / it is done with it.
void summary_card_shown();
void summary_card_done();

// [dev] summary_test: a session counts from 0 s and the card comes 3 s after the friend is gone (one-PC tests).
void summary_set_test(bool on);

}  // namespace cdmp::net
