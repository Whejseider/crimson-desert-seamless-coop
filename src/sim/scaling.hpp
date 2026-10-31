#pragma once
// Party-size scaling for enemy/boss health and damage. Pure logic: no game, no Windows, no Steam.
// The host computes multipliers (host-authoritative) and the game/ layer applies them to enemies.
#include <cstddef>
#include <string>
#include <vector>

namespace cdcoop::sim {

enum class EnemyTier { Regular, Elite, Boss };

// What to do when the party is bigger than the curve has entries for.
enum class Overflow { Clamp, Extrapolate };

struct Curve {
    // Index 0 = 1 player, index 1 = 2 players, ...
    std::vector<double> values{1.0};
    Overflow overflow = Overflow::Extrapolate;

    // Multiplier for a party of `party_size` (>= 1; smaller values are treated as 1).
    double at(int party_size) const;
};

struct TierCurves {
    Curve health;
    Curve damage;
};

// Quality-of-life features change difficulty (shorter cooldowns, more stamina, better gliding...).
// The Comfort Pack reports how strong its active toggles are so scaling can compensate.
struct QolState {
    double cooldown_reduction = 0.0;  // 0.0 = vanilla, 0.5 = cooldowns are half as long
    bool infinite_stamina = false;
};

struct ScalingConfig {
    bool enabled = true;
    TierCurves regular;
    TierCurves elite;
    TierCurves boss;
    // Stagger/posture thresholds scale with health^exponent (1.0 = exactly with health, 0 = never).
    double stagger_exponent = 1.0;
    // Only players within this many metres of an enemy count toward its party size (0 = whole party).
    double count_radius_m = 0.0;
    // Extra enemy health per unit of QoL strength (see qol_strength()).
    double qol_health_compensation = 0.0;
    double min_multiplier = 0.1;
    double max_multiplier = 10.0;

    static ScalingConfig defaults();
};

struct Multipliers {
    double health = 1.0;
    double damage = 1.0;
    double stagger = 1.0;  // apply to absolute poise/stagger/phase-break thresholds
};

// 0..1 summary of how much the active QoL toggles make the game easier.
double qol_strength(const QolState& qol);

Multipliers compute(const ScalingConfig& cfg, int party_size, EnemyTier tier, const QolState& qol = {});

// Scale an absolute threshold (e.g. a boss's "break at 30000 HP" or a poise value) by a multiplier.
// Fractional thresholds (phase at 50% HP) need no scaling: they follow scaled max health automatically.
double scale_absolute_threshold(double threshold, double multiplier);

// Keep an enemy's current health fraction when its max health changes (a player joins/leaves mid-fight).
double rescale_current_health(double current_hp, double old_max_hp, double new_max_hp);

struct Vec3 { double x = 0, y = 0, z = 0; };
// Number of players that count toward one enemy's scaling (always >= 1).
int effective_party_size(const std::vector<Vec3>& player_positions, const Vec3& enemy, double radius_m);

}  // namespace cdcoop::sim
