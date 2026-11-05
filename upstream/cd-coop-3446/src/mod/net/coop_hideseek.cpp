#include "mod/net/coop_hideseek.h"

#include <windows.h>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstring>
#include <format>
#include <mutex>
#include <vector>

#include "mod/log.h"
#include "mod/net/coop_activity.h"
#include "mod/net/puppet_minimap.h"
#include "mod/net/remote_puppet.h"

namespace cdmp::net {
namespace {

constexpr uint8_t kHideType = 26;  // Activity event type: u8 op | u32 round | data
enum Op : uint8_t { kOpStart = 1, kOpFound = 2, kOpNotFound = 3, kOpLeft = 4 };
constexpr float kAreaM = 100.0f;      // the hider stays within this of where the seeker counts (user: "within 100m")
constexpr float kFoundM = 4.0f;       // the seeker this close (flat) finds the hider
constexpr float kFoundUpM = 5.0f;     // ... and within this height (not a floor apart)
constexpr uint32_t kOutMs = 10000;    // outside the area this long gives the round away
constexpr uint32_t kOverMs = 8000;
constexpr uint32_t kVerdictWaitMs = 6000;  // the hider waits this long past the end for the seeker's verdict
enum Phase : uint8_t { kIdle, kWaitStart, kHide, kSeek, kOver };
enum How : uint8_t { kHowNone, kHowFound, kHowNotFound, kHowLeft };

struct State {
    Phase phase = kIdle;
    uint32_t round = 0, since = 0;
    bool seeking = false;           // we count and seek this round
    std::array<float, 3> centre{};  // the hiding area's centre (where the seeker counts)
    uint32_t out_since = 0;         // the hider: outside the area since
    int result = 0;                 // 1 we won the round, 2 they did
    How how = kHowNone;
    uint32_t found_ms = 0;
    bool reported = false;
} g;
struct Pending {
    uint32_t round = 0;
    uint8_t op = 0;
    uint32_t ms = 0;
    std::array<float, 3> centre{};
};
Pending g_start, g_verdict;  // the latest start and verdict from the other player
std::string g_who;
std::mutex g_mutex;
std::atomic<int> g_test{0};
std::atomic<bool> g_hidden{false}, g_active{false};
int g_saved_icon = -1;  // the minimap icon the copy had before we hid it (-1 = not hidden)

uint32_t hide_ms() { return g_test.load() ? 20000u : 45000u; }
uint32_t seek_ms() { return g_test.load() ? 45000u : 180000u; }

float flat(const std::array<float, 3>& a, const std::array<float, 3>& b) { return std::hypot(a[0] - b[0], a[2] - b[2]); }

std::string clock(uint32_t ms) { return std::format("{}:{:02}", ms / 60000, ms / 1000 % 60); }

std::string warmth(float d) {
    return d < 15 ? "Burning hot!" : d < 30 ? "Hot" : d < 60 ? "Warm" : d < 100 ? "Cool" : "Cold";
}

std::vector<uint8_t> message(Op op, uint32_t round, const void* data = nullptr, size_t n = 0) {
    std::vector<uint8_t> m(5 + n);
    m[0] = op;
    std::memcpy(m.data() + 1, &round, 4);
    if (n) std::memcpy(m.data() + 5, data, n);
    return m;
}

// The seeker's minimap must not give the hider away: the copy's icon comes from puppet_minimap, decided when the
// copy loads, so the copy is loaded again without it (and again with it after the game).
void hide_icon(bool hide) {
    if (hide == (g_saved_icon >= 0)) return;
    if (hide) {
        g_saved_icon = puppet_minimap_asked() ? puppet_minimap_offset() : 0;  // no icon drawn: only the pin goes
        if (g_saved_icon > 0) {
            puppet_minimap_set_offset(0);
            puppet_request_reload();
        }
        log::info("Hide and seek: the friend is hidden from us (minimap icon {})", g_saved_icon > 0 ? "off" : "none");
    } else {
        if (g_saved_icon > 0) {
            puppet_minimap_set_offset(g_saved_icon);
            puppet_request_reload();
        }
        g_saved_icon = -1;
        log::info("Hide and seek: the friend is shown again");
    }
}

void begin_round(bool inviter, const std::optional<std::array<float, 3>>& me, uint32_t now,
                 std::vector<std::vector<uint8_t>>& out) {
    // The inviter seeks in rounds 1 and 3, the other player in round 2.
    g.seeking = inviter == (g.round % 2 == 1);
    g.out_since = 0, g.result = 0, g.how = kHowNone, g.found_ms = 0, g.reported = false;
    g.since = now;
    if (g.seeking || g_test.load() == 2) {
        g.centre = me.value_or(std::array<float, 3>{});
        g.phase = kHide;
        if (g.seeking) out.push_back(message(kOpStart, g.round, g.centre.data(), 12));
        log::info("Hide and seek: round {}: {} at ({:.0f}, {:.0f})", g.round, g.seeking ? "we count" : "we hide",
                  g.centre[0], g.centre[2]);
    } else {
        g.phase = kWaitStart;
        log::info("Hide and seek: round {}: we hide; waiting for the seeker's start", g.round);
    }
}

}  // namespace

void hideseek_on_event(const uint8_t* d, size_t n) {
    if (n < 5) return;
    Pending p;
    p.op = d[0];
    std::memcpy(&p.round, d + 1, 4);
    std::lock_guard lock(g_mutex);
    if (p.op == kOpStart && n >= 5 + 12) {
        std::memcpy(p.centre.data(), d + 5, 12);
        g_start = p;
    } else if (p.op == kOpFound || p.op == kOpNotFound || p.op == kOpLeft) {
        if (p.op == kOpFound && n >= 9) std::memcpy(&p.ms, d + 5, 4);
        g_verdict = p;
    }
}

HideSeekTick hideseek_tick(bool on, bool connected, bool inviter, const std::string& who,
                           std::optional<std::array<float, 3>> me, std::optional<std::array<float, 3>> partner) {
    HideSeekTick out;
    const uint32_t now = GetTickCount();
    const int test = g_test.load();
    if (test) inviter = test == 1;
    std::vector<std::vector<uint8_t>> send;
    {
        std::lock_guard lock(g_mutex);
        g_who = who;
        if (!on || !connected) {
            if (g.phase != kIdle) log::info("Hide and seek: over");
            g = State{};
            g_start = Pending{}, g_verdict = Pending{};
            g_hidden = false;
            g_active = false;
            hide_icon(false);
            return out;
        }
        g_active = true;
        // The one-PC test's hider stands still 36 m from where we count (the echo copy is our own character).
        if (test == 1 && g.phase >= kHide) partner = std::array<float, 3>{g.centre[0] + 30, g.centre[1], g.centre[2] + 20};
        const uint32_t t = now - g.since;
        switch (g.phase) {
            case kIdle:
                if (!me) break;
                g.round = 1;
                begin_round(inviter, me, now, send);
                break;
            case kWaitStart:
                if (g_start.round == g.round) {
                    g.centre = g_start.centre;
                    g.phase = kHide, g.since = now;
                    log::info("Hide and seek: round {}: hide! (area centre ({:.0f}, {:.0f}))", g.round, g.centre[0],
                              g.centre[2]);
                }
                break;
            case kHide:
            case kSeek: {
                if (g.phase == kHide && t >= hide_ms()) {
                    g.phase = kSeek, g.since = now;
                    log::info("Hide and seek: round {}: the search begins", g.round);
                    break;
                }
                const bool verdict = g_verdict.round == g.round;
                if (g.seeking) {
                    if (verdict && g_verdict.op == kOpLeft) {
                        g.result = 1, g.how = kHowLeft;
                    } else if (g.phase == kSeek && me && partner && flat(*me, *partner) < kFoundM &&
                               (test == 1 || std::fabs((*me)[1] - (*partner)[1]) < kFoundUpM)) {
                        g.result = 1, g.how = kHowFound, g.found_ms = std::max<uint32_t>(t, 1);
                        send.push_back(message(kOpFound, g.round, &g.found_ms, 4));
                    } else if (g.phase == kSeek && t >= seek_ms()) {
                        g.result = 2, g.how = kHowNotFound;
                        send.push_back(message(kOpNotFound, g.round));
                    }
                } else {
                    if (verdict && g_verdict.op == kOpFound) {
                        g.result = 2, g.how = kHowFound, g.found_ms = g_verdict.ms;
                    } else if (verdict && g_verdict.op == kOpNotFound) {
                        g.result = 1, g.how = kHowNotFound;
                    } else if (g.phase == kSeek && t >= seek_ms() + kVerdictWaitMs) {
                        g.result = 1, g.how = kHowNotFound;  // no word from the seeker: not found
                    } else if (me && flat(*me, g.centre) > kAreaM) {
                        if (!g.out_since) g.out_since = now ? now : 1;
                        if (now - g.out_since >= kOutMs) {
                            g.result = 2, g.how = kHowLeft;
                            send.push_back(message(kOpLeft, g.round));
                        }
                    } else {
                        g.out_since = 0;
                    }
                }
                if (g.result) {
                    log::info("Hide and seek: round {} ends: {} ({})", g.round, g.result == 1 ? "ours" : "theirs",
                              g.how == kHowFound ? "found" : g.how == kHowNotFound ? "not found" : "left the area");
                    g.phase = kOver, g.since = now;
                }
                break;
            }
            case kOver:
                if (t >= kOverMs && me) {
                    ++g.round;
                    begin_round(inviter, me, now, send);
                }
                break;
        }
        if (g.phase == kOver && !g.reported) {
            g.reported = true;
            out.round_result = g.result;
        }
        const bool hidden = g.seeking && (g.phase == kHide || g.phase == kSeek);
        g_hidden = hidden;
        hide_icon(hidden);
        // The running line under the notices.
        const uint32_t tt = now - g.since;
        if (g.phase == kWaitStart) {
            out.banner = std::format("Hide and seek vs {}: {} counts first. Get ready to hide...   ({{Challenge}} stops)", who, who);
        } else if (g.phase == kHide && !g.seeking) {
            const float d = me ? flat(*me, g.centre) : 0;
            out.banner = std::format("Hide!   {} is counting: {}   Stay within {:.0f} m of where they count ({:.0f} m)",
                                     who, clock(hide_ms() - std::min(tt, hide_ms())), kAreaM, d);
        } else if (g.phase == kSeek && g.seeking) {
            const std::string hint = partner && me ? warmth(flat(*me, *partner)) : std::string("...");
            out.banner = std::format("Find {}!   {}   {}   (they hide within {:.0f} m of where you counted)", who,
                                     clock(seek_ms() - std::min(tt, seek_ms())), hint, kAreaM);
        } else if (g.phase == kSeek) {
            const bool close = me && partner && flat(*me, *partner) < 25;
            out.banner = std::format("Stay hidden!   {}   {}", clock(seek_ms() - std::min(tt, seek_ms())),
                                     close ? who + " is close..." : who + " is searching");
        }
        if (!g.seeking && (g.phase == kHide || g.phase == kSeek) && g.out_since) {
            out.banner = std::format("Back into the hiding area!   {} s   (you are {:.0f} m from where {} counts)",
                                     (kOutMs - std::min(now - g.out_since, kOutMs) + 999) / 1000,
                                     me ? flat(*me, g.centre) : 0.0f, who);
        }
    }
    for (const auto& m : send) activity_send(kHideType, m);
    return out;
}

HideSeekHud hideseek_hud() {
    HideSeekHud h;
    std::lock_guard lock(g_mutex);
    const uint32_t t = GetTickCount() - g.since;
    const std::string who = g_who.empty() ? std::string("Your friend") : g_who;
    constexpr unsigned int kGold = 0xFF82C8E8, kRed = 0xFF2830C4;  // ABGR
    if (g.phase == kHide || g.phase == kSeek) {
        h.area = true;
        h.area_centre = g.centre;
        h.area_radius = kAreaM;
        h.outside = !g.seeking && g.out_since != 0;
    }
    if (g.phase == kHide && g.seeking) {
        h.veil = true;
        h.veil_count = std::to_string((hide_ms() - std::min(t, hide_ms()) + 999) / 1000);
        h.veil_line = std::format("Eyes closed. {} is hiding.\nWhen the count ends, search within {:.0f} m of here.",
                                  who, kAreaM);
    } else if (g.phase == kHide && t < 1800) {
        h.title = "Hide!";
        h.subtitle = std::format("{} is counting to {}", who, hide_ms() / 1000);
        h.color = kGold;
        h.alpha = 1.0f - static_cast<float>(t) / 1800.0f;
    } else if (g.phase == kSeek && t < 1800) {
        h.title = g.seeking ? "Seek!" : "Stay Hidden";
        h.subtitle = g.seeking ? std::format("Find {}: warmer and colder tell you how close you are", who)
                               : std::format("{} is looking for you", who);
        h.color = kGold;
        h.alpha = 1.0f - static_cast<float>(t) / 1800.0f;
    } else if (g.phase == kOver) {
        const bool won = g.result == 1;
        if (g.how == kHowFound) {
            h.title = won ? "Found!" : "Found";
            h.subtitle = won ? std::format("You found {} in {}", who, clock(g.found_ms))
                             : std::format("{} found you in {}", who, clock(g.found_ms));
        } else if (g.how == kHowNotFound) {
            h.title = won ? "Well Hidden" : "Not Found";
            h.subtitle = won ? std::format("{} never found you", who) : std::format("{} stayed hidden", who);
        } else {
            h.title = won ? "Victory" : "Out of Bounds";
            h.subtitle = won ? std::format("{} left the hiding area", who) : std::string("You left the hiding area");
        }
        h.color = won ? kGold : kRed;
        h.alpha = t < kOverMs - 1000 ? 1.0f : std::max(0.0f, static_cast<float>(kOverMs - t) / 1000.0f);
    }
    return h;
}

bool hideseek_partner_hidden() { return g_hidden.load(); }
bool hideseek_active() { return g_active.load(); }
void hideseek_set_test(int mode) { g_test = mode; }

}  // namespace cdmp::net
