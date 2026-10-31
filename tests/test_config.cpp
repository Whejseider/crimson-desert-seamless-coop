#include "sim/config.hpp"
#include "test.hpp"

using namespace cdcoop::sim;

TEST(config_shipped_files_load_without_errors) {
    const auto cfg = load_config(CDCOOP_CONFIG_DIR);
    for (const auto& e : cfg.errors) std::printf("  config error: %s\n", e.c_str());
    CHECK(cfg.errors.empty());
    CHECK(cfg.general.enabled);
    CHECK_NEAR(compute(cfg.scaling, 2, EnemyTier::Boss).health, 1.7, 1e-9);
    CHECK(cfg.loot.mode == LootMode::Instanced);
    CHECK(cfg.progression.guest_save_mode == GuestSaveMode::SeparateSlot);
    CHECK(!cfg.comfort.enabled);  // Comfort Pack ships off: vanilla balance unless the host opts in
}

TEST(config_bad_values_report_errors_and_keep_defaults) {
    CoopConfig cfg;
    apply_toml(cfg, "scaling.toml", "boss.health = [1.0, -2.0]\nmin_multiplier = 5.0\nmax_multiplier = 1.0\n");
    CHECK(cfg.errors.size() == 2);
    CHECK_NEAR(cfg.scaling.boss.health.at(2), 1.7, 1e-9);
    CHECK_NEAR(cfg.scaling.max_multiplier, 10.0, 1e-9);
}

TEST(config_syntax_error_is_reported_with_line) {
    CoopConfig cfg;
    apply_toml(cfg, "loot.toml", "mode = \"instanced\"\nthis is not toml\n");
    CHECK(cfg.errors.size() == 1);
    CHECK(cfg.errors[0].find("line 2") != std::string::npos);
}

TEST(config_loot_and_progression_overrides) {
    CoopConfig cfg;
    apply_toml(cfg, "loot.toml", "mode = \"need_greed\"\ncurrency_to_all = false\n");
    apply_toml(cfg, "progression.toml", "[shared]\nside_quests = false\n[saves]\nguest_save_mode = \"never_write\"\n");
    CHECK(cfg.errors.empty());
    CHECK(cfg.loot.mode == LootMode::NeedGreed);
    CHECK(!cfg.loot.currency_to_all);
    CHECK(cfg.progression.of(Category::SideQuests) == Policy::Personal);
    CHECK(cfg.progression.guest_save_mode == GuestSaveMode::NeverWrite);
}

TEST(config_comfort_feeds_scaling_qol) {
    CoopConfig cfg;
    apply_toml(cfg, "comfort.toml", "enabled = true\n[combat]\nskill_cooldown_multiplier = 0.5\ninfinite_stamina = true\n");
    CHECK(cfg.errors.empty());
    const auto q = cfg.comfort.qol_state();
    CHECK_NEAR(q.cooldown_reduction, 0.5, 1e-9);
    CHECK(q.infinite_stamina);
}
