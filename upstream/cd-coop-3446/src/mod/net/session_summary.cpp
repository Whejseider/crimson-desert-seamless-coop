#include "mod/net/session_summary.h"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <format>
#include <mutex>

#include "mod/log.h"

namespace cdmp::net {
namespace {

constexpr uint32_t kGoneMs = 20000;       // gone this long without coming back: the session is over
constexpr uint32_t kMinMs = 120000;       // shorter sessions show no card
constexpr float kTogetherM = 150.0f;      // within this of each other, our travel counts as "together"
constexpr float kStepMaxM = 60.0f;        // a jump bigger than this per tick is a teleport / load, not travel

constexpr size_t kStats = static_cast<size_t>(SummaryStat::Count);

std::mutex g_mutex;
std::atomic<bool> g_test{false};
bool g_running = false;          // a session is under way (connected, or gone less than kGoneMs)
uint32_t g_connected_ms = 0;     // time connected in this session
uint32_t g_last_tick = 0;
uint32_t g_gone_since = 0;       // 0 = connected
uint32_t g_counts[kStats] = {};
double g_together_m = 0;
std::optional<std::array<float, 3>> g_last_me;
std::string g_partner;
SummaryCard g_card;              // the finished session's card (pending until shown and done)
bool g_card_shown = false;

std::string duration(uint32_t ms) {
    const uint32_t min = (ms + 30000) / 60000;
    if (min < 60) return std::format("{} min", std::max<uint32_t>(min, 1));
    return min % 60 ? std::format("{} h {} min", min / 60, min % 60) : std::format("{} h", min / 60);
}

std::string distance(double m) {
    return m < 1000 ? std::format("{:.0f} m", m) : std::format("{:.1f} km", m / 1000.0);
}

uint32_t count(SummaryStat s) { return g_counts[static_cast<size_t>(s)]; }

void finish_session() {
    g_running = false;
    const bool test = g_test.load();
    log::info("Summary: session with {} over: {} together, kills {}/{}, matches {}-{}, revives {}/{}, quest steps {}, "
              "bosses {}, {:.0f} m side by side",
              g_partner, duration(g_connected_ms), count(SummaryStat::OurKill), count(SummaryStat::TheirKill),
              count(SummaryStat::MatchWon), count(SummaryStat::MatchLost), count(SummaryStat::WeRevived),
              count(SummaryStat::TheyRevivedUs), count(SummaryStat::QuestStep), count(SummaryStat::Boss),
              g_together_m);
    if (g_connected_ms < kMinMs && !test) {
        log::info("Summary: too short for a card");
        return;
    }
    const std::string who = g_partner.empty() ? std::string("Your friend") : g_partner;
    SummaryCard c;
    c.pending = true;
    c.partner = who;
    c.title = "Journey Together";
    c.subtitle = std::format("with {}   -   {}", who, duration(g_connected_ms));
    c.rows.push_back({"Enemies defeated", std::to_string(count(SummaryStat::OurKill)),
                      std::to_string(count(SummaryStat::TheirKill))});
    if (count(SummaryStat::Boss)) c.rows.push_back({"Bosses brought down together", std::to_string(count(SummaryStat::Boss)), ""});
    if (count(SummaryStat::WeRevived) || count(SummaryStat::TheyRevivedUs)) {
        c.rows.push_back({"Revived the other", std::to_string(count(SummaryStat::WeRevived)),
                          std::to_string(count(SummaryStat::TheyRevivedUs))});
    }
    if (count(SummaryStat::MatchWon) || count(SummaryStat::MatchLost)) {
        c.rows.push_back({"Challenges won", std::to_string(count(SummaryStat::MatchWon)),
                          std::to_string(count(SummaryStat::MatchLost))});
    }
    if (count(SummaryStat::QuestStep)) {
        c.rows.push_back({"Quest steps finished together", std::to_string(count(SummaryStat::QuestStep)), ""});
    }
    if (g_together_m >= 50) c.rows.push_back({"Travelled side by side", distance(g_together_m), ""});
    g_card = std::move(c);
    g_card_shown = false;
}

}  // namespace

void summary_note(SummaryStat stat) {
    std::lock_guard lock(g_mutex);
    if (!g_running || stat == SummaryStat::Count) return;
    ++g_counts[static_cast<size_t>(stat)];
}

void summary_tick(bool connected, const std::string& partner_name, std::optional<std::array<float, 3>> me,
                  std::optional<std::array<float, 3>> partner, uint32_t now_ms) {
    std::lock_guard lock(g_mutex);
    const uint32_t dt = g_last_tick ? std::min<uint32_t>(now_ms - g_last_tick, 1000) : 0;
    g_last_tick = now_ms;
    if (connected) {
        if (!g_running) {
            g_running = true;
            g_connected_ms = 0;
            std::fill(std::begin(g_counts), std::end(g_counts), 0u);
            g_together_m = 0;
            g_last_me.reset();
            log::info("Summary: a session with {} begins", partner_name);
        } else if (g_gone_since) {
            log::info("Summary: {} is back; the same session goes on", partner_name);
        }
        g_gone_since = 0;
        if (!partner_name.empty()) g_partner = partner_name;
        g_connected_ms += dt;
        // Distance travelled side by side: our own steps while the friend is near.
        if (me && g_last_me) {
            const float step = std::hypot((*me)[0] - (*g_last_me)[0], (*me)[2] - (*g_last_me)[2]);
            const bool near = partner && std::hypot((*me)[0] - (*partner)[0], (*me)[2] - (*partner)[2]) < kTogetherM;
            if (near && step < kStepMaxM) g_together_m += step;
        }
        g_last_me = me;
        return;
    }
    g_last_me.reset();
    if (!g_running) return;
    if (!g_gone_since) g_gone_since = now_ms ? now_ms : 1;
    if (now_ms - g_gone_since >= (g_test.load() ? 3000u : kGoneMs)) finish_session();
}

SummaryCard summary_card() {
    std::lock_guard lock(g_mutex);
    return g_card;
}

void summary_card_shown() {
    std::lock_guard lock(g_mutex);
    if (g_card.pending && !g_card_shown) {
        g_card_shown = true;
        log::info("Summary: the card is on screen");
    }
}

void summary_card_done() {
    std::lock_guard lock(g_mutex);
    g_card = SummaryCard{};
    g_card_shown = false;
}

void summary_set_test(bool on) { g_test = on; }

}  // namespace cdmp::net
