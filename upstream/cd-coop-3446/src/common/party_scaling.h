#pragma once
// Party-size enemy toughness (Co-op+, brief 6.5): with a guest in the host's
// world, the host's enemies take only 1/k of the damage dealt to them, which is
// the same as having k times the health, without touching the game's health pool.
//
//   - the guest's blows (guest_hits.cpp) are applied already divided by k;
//   - the host's own blows are native: after each one the host gives the enemy
//     back (1 - 1/k) of what it lost (party_scaling.cpp, VaryStatCheatReq).
//
// Blows the mod itself injected (the guest's) show up in the game's VaryHPAck
// exactly like native ones, so they are noted in a ledger first and skipped by
// the give-back; otherwise the guest's damage would be scaled twice.
//
// Pure logic, no Windows: unit-tested in tests/tests.cpp.

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <deque>

namespace cdmp::scaling {

struct Settings {
    bool enabled = false;
    float health_2p = 1.7f;  // brief 6.5 starting curve: x1.0 / 1.7 / 2.3 / 2.8 for 1-4 players
};

inline constexpr float kMinMultiplier = 1.0f;
inline constexpr float kMaxMultiplier = 4.0f;

// Enemy toughness for a party of `players` (1 = alone: always 1).
inline float health_multiplier(const Settings& s, int players) {
    if (!s.enabled || players < 2 || !(s.health_2p > 0.0f)) return 1.0f;
    return std::clamp(s.health_2p, kMinMultiplier, kMaxMultiplier);
}

// A guest blow of `damage` whole HP, scaled. Never 0 for a real blow: a hit that
// lands always costs at least 1 HP, as in the game.
inline int32_t scale_damage(int32_t damage, float k) {
    if (damage <= 0) return 0;
    if (!(k > 1.0f)) return damage;
    return std::max<int32_t>(1, static_cast<int32_t>(std::lround(static_cast<double>(damage) / k)));
}

// Whole HP to give back after a native blow took the enemy from `before` to
// `after` (both x1000, as VaryHPAck carries them). 0 when there is nothing to do:
// no loss, the blow killed it (a dead enemy is never healed), or less than 1 HP.
// Always strictly less than the loss, so a blow can never end up healing.
inline int32_t heal_back(int64_t before_x1000, int64_t after_x1000, float k) {
    if (!(k > 1.0f) || after_x1000 <= 0 || before_x1000 <= after_x1000) return 0;
    const double loss = static_cast<double>(before_x1000 - after_x1000) / 1000.0;
    // What the blow should have cost, rounded the way scale_damage rounds a guest
    // blow, so the same blow costs the same whoever lands it. Computing
    // loss * (1 - 1/k) directly floors 6.9999 to 6 with k = 1.7f.
    const double net = static_cast<double>(std::lround(loss / k));
    const double back = std::floor(loss - std::max(net, 1.0));
    if (back < 1.0 || back >= loss) return 0;
    return static_cast<int32_t>(std::min(back, 1.0e6));
}

// HP losses the mod caused itself, so the give-back can tell them from native blows.
class InjectedLedger {
public:
    static constexpr uint64_t kTtlMs = 3000;
    static constexpr size_t kMax = 256;

    void note(uint32_t enemy, int32_t loss_hp, uint64_t now_ms) {
        if (loss_hp <= 0) return;
        entries_.push_back({enemy, loss_hp, now_ms});
        while (entries_.size() > kMax) entries_.pop_front();
    }

    // True (and forgotten) if a loss of `loss_x1000` on `enemy` is one we noted.
    // Matches within 1 HP: the game rounds the x1000 values.
    bool consume(uint32_t enemy, int64_t loss_x1000, uint64_t now_ms) {
        expire(now_ms);
        for (auto it = entries_.begin(); it != entries_.end(); ++it) {
            if (it->enemy != enemy) continue;
            const int64_t noted = static_cast<int64_t>(it->loss_hp) * 1000;
            if (loss_x1000 >= noted - 1000 && loss_x1000 <= noted + 1000) {
                entries_.erase(it);
                return true;
            }
        }
        return false;
    }

    size_t size() const { return entries_.size(); }

private:
    struct Entry {
        uint32_t enemy;
        int32_t loss_hp;
        uint64_t at_ms;
    };
    void expire(uint64_t now_ms) {
        while (!entries_.empty() && now_ms - entries_.front().at_ms > kTtlMs) entries_.pop_front();
    }
    std::deque<Entry> entries_;
};

}  // namespace cdmp::scaling
