#include "sim/scaling.hpp"

#include <algorithm>
#include <cmath>

namespace cdcoop::sim {

double Curve::at(int party_size) const {
    if (values.empty()) return 1.0;
    const int n = static_cast<int>(values.size());
    const int idx = std::max(party_size, 1) - 1;
    if (idx < n) return values[static_cast<std::size_t>(idx)];
    if (overflow == Overflow::Clamp || n == 1) return values.back();
    const double step = values[n - 1] - values[n - 2];
    return values.back() + step * (idx - (n - 1));
}

ScalingConfig ScalingConfig::defaults() {
    ScalingConfig c;
    // Starting curve from the brief (health x1.0 / 1.7 / 2.3 / 2.8 for 1-4 players). To be tuned in play tests.
    c.regular.health.values = {1.0, 1.5, 2.0, 2.4};
    c.regular.damage.values = {1.0, 1.1, 1.2, 1.3};
    c.elite.health.values = {1.0, 1.6, 2.2, 2.6};
    c.elite.damage.values = {1.0, 1.15, 1.25, 1.35};
    c.boss.health.values = {1.0, 1.7, 2.3, 2.8};
    c.boss.damage.values = {1.0, 1.15, 1.3, 1.4};
    return c;
}

double qol_strength(const QolState& qol) {
    double s = std::clamp(qol.cooldown_reduction, 0.0, 1.0) * 0.6;
    if (qol.infinite_stamina) s += 0.4;
    return std::clamp(s, 0.0, 1.0);
}

Multipliers compute(const ScalingConfig& cfg, int party_size, EnemyTier tier, const QolState& qol) {
    if (!cfg.enabled) return {};
    const TierCurves& t = tier == EnemyTier::Boss ? cfg.boss : tier == EnemyTier::Elite ? cfg.elite : cfg.regular;
    const auto clampm = [&](double v) { return std::clamp(v, cfg.min_multiplier, cfg.max_multiplier); };
    Multipliers m;
    m.health = clampm(t.health.at(party_size) * (1.0 + cfg.qol_health_compensation * qol_strength(qol)));
    m.damage = clampm(t.damage.at(party_size));
    m.stagger = clampm(std::pow(m.health, cfg.stagger_exponent));
    return m;
}

double scale_absolute_threshold(double threshold, double multiplier) { return threshold * multiplier; }

double rescale_current_health(double current_hp, double old_max_hp, double new_max_hp) {
    if (old_max_hp <= 0.0) return new_max_hp;
    const double frac = std::clamp(current_hp / old_max_hp, 0.0, 1.0);
    return frac * new_max_hp;
}

int effective_party_size(const std::vector<Vec3>& players, const Vec3& e, double radius_m) {
    if (players.empty()) return 1;
    if (radius_m <= 0.0) return static_cast<int>(players.size());
    const double r2 = radius_m * radius_m;
    int n = 0;
    for (const Vec3& p : players) {
        const double dx = p.x - e.x, dy = p.y - e.y, dz = p.z - e.z;
        if (dx * dx + dy * dy + dz * dz <= r2) ++n;
    }
    return std::max(n, 1);
}

}  // namespace cdcoop::sim
