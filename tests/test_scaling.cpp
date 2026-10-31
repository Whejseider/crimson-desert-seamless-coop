#include "sim/scaling.hpp"
#include "test.hpp"

using namespace cdcoop::sim;

TEST(scaling_boss_curve_matches_brief) {
    const auto cfg = ScalingConfig::defaults();
    CHECK_NEAR(compute(cfg, 1, EnemyTier::Boss).health, 1.0, 1e-9);
    CHECK_NEAR(compute(cfg, 2, EnemyTier::Boss).health, 1.7, 1e-9);
    CHECK_NEAR(compute(cfg, 3, EnemyTier::Boss).health, 2.3, 1e-9);
    CHECK_NEAR(compute(cfg, 4, EnemyTier::Boss).health, 2.8, 1e-9);
}

TEST(scaling_solo_is_vanilla_for_every_tier) {
    const auto cfg = ScalingConfig::defaults();
    for (auto tier : {EnemyTier::Regular, EnemyTier::Elite, EnemyTier::Boss}) {
        const auto m = compute(cfg, 1, tier);
        CHECK_NEAR(m.health, 1.0, 1e-9);
        CHECK_NEAR(m.damage, 1.0, 1e-9);
        CHECK_NEAR(m.stagger, 1.0, 1e-9);
    }
}

TEST(scaling_zero_or_negative_party_counts_as_one) {
    const auto cfg = ScalingConfig::defaults();
    CHECK_NEAR(compute(cfg, 0, EnemyTier::Boss).health, 1.0, 1e-9);
    CHECK_NEAR(compute(cfg, -3, EnemyTier::Boss).health, 1.0, 1e-9);
}

TEST(scaling_overflow_extrapolates_or_clamps) {
    Curve c; c.values = {1.0, 1.7, 2.3, 2.8};
    c.overflow = Overflow::Extrapolate;
    CHECK_NEAR(c.at(5), 3.3, 1e-9);
    CHECK_NEAR(c.at(6), 3.8, 1e-9);
    c.overflow = Overflow::Clamp;
    CHECK_NEAR(c.at(6), 2.8, 1e-9);
}

TEST(scaling_disabled_returns_identity) {
    auto cfg = ScalingConfig::defaults();
    cfg.enabled = false;
    CHECK_NEAR(compute(cfg, 4, EnemyTier::Boss).health, 1.0, 1e-9);
}

TEST(scaling_stagger_follows_health_by_exponent) {
    auto cfg = ScalingConfig::defaults();
    CHECK_NEAR(compute(cfg, 4, EnemyTier::Boss).stagger, 2.8, 1e-9);
    cfg.stagger_exponent = 0.5;
    CHECK_NEAR(compute(cfg, 4, EnemyTier::Boss).stagger, std::sqrt(2.8), 1e-9);
    cfg.stagger_exponent = 0.0;
    CHECK_NEAR(compute(cfg, 4, EnemyTier::Boss).stagger, 1.0, 1e-9);
}

TEST(scaling_clamps_to_limits) {
    auto cfg = ScalingConfig::defaults();
    cfg.max_multiplier = 2.0;
    CHECK_NEAR(compute(cfg, 4, EnemyTier::Boss).health, 2.0, 1e-9);
}

TEST(scaling_qol_compensation) {
    auto cfg = ScalingConfig::defaults();
    cfg.qol_health_compensation = 0.25;
    QolState q; q.cooldown_reduction = 0.5; q.infinite_stamina = true;  // strength = 0.3 + 0.4 = 0.7
    CHECK_NEAR(qol_strength(q), 0.7, 1e-9);
    CHECK_NEAR(compute(cfg, 2, EnemyTier::Boss, q).health, 1.7 * (1 + 0.25 * 0.7), 1e-9);
    CHECK_NEAR(compute(cfg, 2, EnemyTier::Boss, {}).health, 1.7, 1e-9);
}

TEST(scaling_rescale_keeps_health_fraction) {
    CHECK_NEAR(rescale_current_health(500, 1000, 1700), 850, 1e-9);
    CHECK_NEAR(rescale_current_health(1200, 1000, 1700), 1700, 1e-9);  // overheal clamps
    CHECK_NEAR(rescale_current_health(10, 0, 1700), 1700, 1e-9);
    CHECK_NEAR(scale_absolute_threshold(30000, 1.7), 51000, 1e-9);
}

TEST(scaling_effective_party_size_by_radius) {
    std::vector<Vec3> players = {{0, 0, 0}, {10, 0, 0}, {100, 0, 0}};
    CHECK(effective_party_size(players, {0, 0, 0}, 0) == 3);
    CHECK(effective_party_size(players, {0, 0, 0}, 50) == 2);
    CHECK(effective_party_size(players, {1000, 0, 0}, 50) == 1);  // never below 1
    CHECK(effective_party_size({}, {0, 0, 0}, 50) == 1);
}
