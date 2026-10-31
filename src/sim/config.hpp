#pragma once
// All of the mod's settings, loaded from the TOML files in config/. Missing files or keys keep their defaults;
// bad values are reported in `errors` and also keep their defaults, so a typo never stops the game.
#include <filesystem>
#include <string>
#include <vector>

#include "sim/loot.hpp"
#include "sim/progression.hpp"
#include "sim/scaling.hpp"

namespace cdcoop::sim {

struct GeneralConfig {
    bool enabled = true;          // false = "vanilla mode": the DLL loads, logs one line, and does nothing
    std::string menu_hotkey = "F8";
    std::string log_level = "info";
    int max_players = 4;
    // The mod only runs inside this executable (ASI loaders can also load into launchers/crash reporters).
    std::string expected_exe = "CrimsonDesert.exe";
};

struct UiConfig {
    bool nameplates = true;
    double nameplate_fade_start_m = 40.0;
    double nameplate_fade_end_m = 80.0;
    bool nameplates_through_walls = false;
    bool map_markers = true;
    bool party_hud = true;
};

// Quality-of-life "Comfort Pack". Everything off by default except where noted; each value is a multiplier.
// Gameplay-affecting values come from the HOST's file in a session so every player plays the same rules.
struct ComfortConfig {
    bool enabled = false;
    bool use_host_settings_in_coop = true;
    int extra_inventory_slots = 0;
    double stack_size_multiplier = 1.0;
    double weight_limit_multiplier = 1.0;
    double glide_stamina_cost_multiplier = 1.0;
    double glide_speed_multiplier = 1.0;
    double skill_cooldown_multiplier = 1.0;  // 0.5 = cooldowns half as long
    bool infinite_stamina = false;
    bool free_fast_travel = false;
    double auto_loot_radius_m = 0.0;  // 0 = off

    QolState qol_state() const;
};

struct CoopConfig {
    GeneralConfig general;
    ScalingConfig scaling = ScalingConfig::defaults();
    LootRules loot;
    ProgressionRules progression = ProgressionRules::defaults();
    UiConfig ui;
    ComfortConfig comfort;
    std::vector<std::string> errors;
};

// Reads cdcoop.toml, scaling.toml, loot.toml, progression.toml, comfort.toml from `dir` (each optional).
CoopConfig load_config(const std::filesystem::path& dir);

// Same, from in-memory TOML text (used by tests and by the host->guest settings sync).
void apply_toml(CoopConfig& cfg, const std::string& file_label, std::string_view toml_text);

}  // namespace cdcoop::sim
