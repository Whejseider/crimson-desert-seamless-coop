#pragma once
// Co-op+ duels (user 2026-11-02: "a duel system where you can duel your friends in a 1v1 PVP match ... inside of a set
// radius ... if you get too far away from the dueling location for too long you automatically lose"). D41.
//
// A duel is a challenge game (coop_activity.h, Shift+F5 / chord + D-pad Up, "Duel" in the picker). Once accepted:
//   * the duel ground: a circle of kRadiusM around the point between the two players, marked on the ground;
//   * each round: 3, 2, 1, "Fight!"; the first brought to their knees (kFloor of their health, never killed) loses;
//     out of the circle for kOutMs in a row also loses; then both are healed back to their health at the round's
//     start and the next round begins; the series score is the challenge's;
//   * hits: the two characters cannot hit each other in the game (each is a friendly copy in the other's world), so
//     the attacker's mod judges them: our own attack (a StartActionReq whose action is a known attack, guest_hits.h),
//     kImpactMs later, with the other player's character (as drawn on our screen) within kReachM in front of us
//     (kArcDeg either side of our facing). The hit goes to them; their mod applies it through their own server
//     (VaryStatByAccInputReq) with a stagger learnt from our fights (player_hits.h), so they flinch on both screens.
//
// Activity event type 12 (kDuel), u8 sub | u32 round | ...:
//   1 round start (from the inviter)  f32 centre x, y, z | f32 radius
//   2 hit on you                       PlayerHit payload (player_hits.h hits_make_pvp)
//   3 I lost the round                 u8 why (1 brought down, 2 out of the duel ground)

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace cdmp::net {

// The challenge "stage" of a duel (not a game stage: a key no stage uses).
inline constexpr uint32_t kDuelStage = 0xD0E10001;

// Our client requests: our swings.
void duel_observe_c2s(const uint8_t* packet, size_t size);

// A kDuel event from the other player (payload after the Activity header).
void duel_on_event(const uint8_t* data, size_t n);

struct DuelTick {
    std::string banner;  // the running line ("" = none)
    std::string toast;   // one-off notice
    int round_result = 0;  // this tick a round ended: 1 we won it, 2 they did
};

// Every network loop. `on`: a duel challenge is on; `inviter`: we sent it (we place the duel ground).
DuelTick duel_tick(bool on, bool inviter, bool connected, std::optional<std::array<float, 3>> me,
                   std::optional<std::array<float, 3>> partner, const std::string& who, uint32_t now);

// What the overlay draws (render/hud.cpp).
struct DuelHud {
    bool active = false;
    std::array<float, 3> centre{};
    float radius = 0;
    std::string title;      // big centred title ("3", "Fight!", "Victory", "Defeat"; "" = none)
    std::string subtitle;
    unsigned int title_color = 0;
    float title_alpha = 0;
    float out_left_s = -1;  // seconds left to get back in the duel ground (-1 = inside)
    float partner_hp = -1;  // the opponent's health 0..1 (-1 unknown)
};
DuelHud duel_hud();

// [dev] duel_test: one-PC test with the echo peer: our hits on its copy of us come back as hits on us, and its echo
// of our own "lost" is ignored.
void duel_set_test(bool on);

}  // namespace cdmp::net
