#include "sim/config.hpp"

#include <fstream>
#include <sstream>

#include <toml.hpp>

namespace cdcoop::sim {

QolState ComfortConfig::qol_state() const {
    QolState q;
    if (!enabled) return q;
    q.cooldown_reduction = skill_cooldown_multiplier < 1.0 ? 1.0 - skill_cooldown_multiplier : 0.0;
    q.infinite_stamina = infinite_stamina;
    return q;
}

namespace {

struct Ctx {
    CoopConfig& cfg;
    std::string file;
    void err(const std::string& key, const std::string& what) { cfg.errors.push_back(file + ": " + key + ": " + what); }
};

template <class T>
void get(Ctx& c, const toml::table& t, std::string_view path, T& out) {
    auto node = t.at_path(path);
    if (!node) return;
    if constexpr (std::is_same_v<T, bool>) {
        if (auto v = node.value<bool>()) out = *v; else c.err(std::string(path), "expected true/false");
    } else if constexpr (std::is_same_v<T, std::string>) {
        if (auto v = node.value<std::string>()) out = *v; else c.err(std::string(path), "expected a string");
    } else if constexpr (std::is_integral_v<T>) {
        if (auto v = node.value<std::int64_t>()) out = static_cast<T>(*v); else c.err(std::string(path), "expected a whole number");
    } else {
        if (auto v = node.value<double>()) out = *v; else c.err(std::string(path), "expected a number");
    }
}

void get_curve(Ctx& c, const toml::table& t, std::string_view path, Curve& out) {
    auto node = t.at_path(path);
    if (!node) return;
    const toml::array* arr = node.as_array();
    if (!arr || arr->empty()) return c.err(std::string(path), "expected a list of numbers, e.g. [1.0, 1.7, 2.3, 2.8]");
    std::vector<double> vals;
    for (const auto& e : *arr) {
        auto v = e.value<double>();
        if (!v || *v <= 0.0) return c.err(std::string(path), "every entry must be a number above 0");
        vals.push_back(*v);
    }
    out.values = std::move(vals);
}

void get_tier(Ctx& c, const toml::table& t, const std::string& name, TierCurves& tier, Overflow overflow) {
    get_curve(c, t, name + ".health", tier.health);
    get_curve(c, t, name + ".damage", tier.damage);
    tier.health.overflow = tier.damage.overflow = overflow;
}

void apply_general(Ctx& c, const toml::table& t) {
    auto& g = c.cfg.general;
    get(c, t, "general.enabled", g.enabled);
    get(c, t, "general.menu_hotkey", g.menu_hotkey);
    get(c, t, "general.log_level", g.log_level);
    get(c, t, "general.max_players", g.max_players);
    get(c, t, "general.expected_exe", g.expected_exe);
    if (g.max_players < 2 || g.max_players > 8) { c.err("general.max_players", "must be 2..8"); g.max_players = 4; }
    auto& u = c.cfg.ui;
    get(c, t, "ui.nameplates", u.nameplates);
    get(c, t, "ui.nameplate_fade_start_m", u.nameplate_fade_start_m);
    get(c, t, "ui.nameplate_fade_end_m", u.nameplate_fade_end_m);
    get(c, t, "ui.nameplates_through_walls", u.nameplates_through_walls);
    get(c, t, "ui.map_markers", u.map_markers);
    get(c, t, "ui.party_hud", u.party_hud);
}

void apply_scaling(Ctx& c, const toml::table& t) {
    auto& s = c.cfg.scaling;
    get(c, t, "enabled", s.enabled);
    std::string overflow = "extrapolate";
    get(c, t, "overflow", overflow);
    Overflow ov = Overflow::Extrapolate;
    if (overflow == "clamp") ov = Overflow::Clamp;
    else if (overflow != "extrapolate") c.err("overflow", "must be \"clamp\" or \"extrapolate\"");
    get_tier(c, t, "regular", s.regular, ov);
    get_tier(c, t, "elite", s.elite, ov);
    get_tier(c, t, "boss", s.boss, ov);
    get(c, t, "stagger_exponent", s.stagger_exponent);
    get(c, t, "count_radius_m", s.count_radius_m);
    get(c, t, "qol_health_compensation", s.qol_health_compensation);
    get(c, t, "min_multiplier", s.min_multiplier);
    get(c, t, "max_multiplier", s.max_multiplier);
    if (s.min_multiplier <= 0 || s.min_multiplier > s.max_multiplier) {
        c.err("min_multiplier/max_multiplier", "need 0 < min <= max");
        s.min_multiplier = 0.1; s.max_multiplier = 10.0;
    }
}

void apply_loot(Ctx& c, const toml::table& t) {
    auto& l = c.cfg.loot;
    std::string mode;
    get(c, t, "mode", mode);
    if (mode == "instanced") l.mode = LootMode::Instanced;
    else if (mode == "shared_pile") l.mode = LootMode::SharedPile;
    else if (mode == "round_robin") l.mode = LootMode::RoundRobin;
    else if (mode == "need_greed") l.mode = LootMode::NeedGreed;
    else if (!mode.empty()) c.err("mode", "must be instanced, shared_pile, round_robin or need_greed");
    get(c, t, "currency_to_all", l.currency_to_all);
    get(c, t, "quest_items_to_all", l.quest_items_to_all);
    get(c, t, "nearby_radius_m", l.nearby_radius_m);
}

void apply_progression(Ctx& c, const toml::table& t) {
    auto& p = c.cfg.progression;
    if (auto shared = t["shared"].as_table()) {
        for (auto&& [k, v] : *shared) {
            Category cat;
            if (!parse_category(k.str(), cat)) { c.err("shared." + std::string(k.str()), "unknown category"); continue; }
            if (auto b = v.value<bool>()) p.policy[static_cast<std::size_t>(cat)] = *b ? Policy::Shared : Policy::Personal;
            else c.err("shared." + std::string(k.str()), "expected true/false");
        }
    }
    get(c, t, "guest_progress_counts_for_host", p.guest_progress_counts_for_host);
    get(c, t, "saves.backup_before_session", p.backup_saves_before_session);
    std::string mode;
    get(c, t, "saves.guest_save_mode", mode);
    if (mode == "separate_slot") p.guest_save_mode = GuestSaveMode::SeparateSlot;
    else if (mode == "ask_each_time") p.guest_save_mode = GuestSaveMode::AskEachTime;
    else if (mode == "never_write") p.guest_save_mode = GuestSaveMode::NeverWrite;
    else if (!mode.empty()) c.err("saves.guest_save_mode", "must be separate_slot, ask_each_time or never_write");
}

void apply_comfort(Ctx& c, const toml::table& t) {
    auto& m = c.cfg.comfort;
    get(c, t, "enabled", m.enabled);
    get(c, t, "use_host_settings_in_coop", m.use_host_settings_in_coop);
    get(c, t, "inventory.extra_slots", m.extra_inventory_slots);
    get(c, t, "inventory.stack_size_multiplier", m.stack_size_multiplier);
    get(c, t, "inventory.weight_limit_multiplier", m.weight_limit_multiplier);
    get(c, t, "gliding.stamina_cost_multiplier", m.glide_stamina_cost_multiplier);
    get(c, t, "gliding.speed_multiplier", m.glide_speed_multiplier);
    get(c, t, "combat.skill_cooldown_multiplier", m.skill_cooldown_multiplier);
    get(c, t, "combat.infinite_stamina", m.infinite_stamina);
    get(c, t, "travel.free_fast_travel", m.free_fast_travel);
    get(c, t, "loot.auto_loot_radius_m", m.auto_loot_radius_m);
    if (m.skill_cooldown_multiplier < 0.05 || m.skill_cooldown_multiplier > 1.0) {
        c.err("combat.skill_cooldown_multiplier", "must be 0.05..1.0");
        m.skill_cooldown_multiplier = 1.0;
    }
    if (m.extra_inventory_slots < 0 || m.extra_inventory_slots > 500) {
        c.err("inventory.extra_slots", "must be 0..500");
        m.extra_inventory_slots = 0;
    }
}

}  // namespace

void apply_toml(CoopConfig& cfg, const std::string& file_label, std::string_view text) {
    toml::parse_result res = toml::parse(text, std::string_view{file_label});
    if (!res) {
        std::ostringstream os;
        os << file_label << ": line " << res.error().source().begin.line << ": " << res.error().description();
        cfg.errors.push_back(os.str());
        return;
    }
    const toml::table& t = res.table();
    Ctx c{cfg, file_label};
    if (file_label == "cdcoop.toml") apply_general(c, t);
    else if (file_label == "scaling.toml") apply_scaling(c, t);
    else if (file_label == "loot.toml") apply_loot(c, t);
    else if (file_label == "progression.toml") apply_progression(c, t);
    else if (file_label == "comfort.toml") apply_comfort(c, t);
    else cfg.errors.push_back(file_label + ": unknown config file");
}

CoopConfig load_config(const std::filesystem::path& dir) {
    CoopConfig cfg;
    for (const char* name : {"cdcoop.toml", "scaling.toml", "loot.toml", "progression.toml", "comfort.toml"}) {
        std::ifstream in(dir / name, std::ios::binary);
        if (!in) continue;
        std::stringstream ss;
        ss << in.rdbuf();
        apply_toml(cfg, name, ss.str());
    }
    return cfg;
}

}  // namespace cdcoop::sim
