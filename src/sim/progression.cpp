#include "sim/progression.hpp"

#include <algorithm>

namespace cdcoop::sim {

namespace {
constexpr std::array<std::string_view, static_cast<std::size_t>(Category::Count)> kNames = {
    "story", "side_quests", "region_discovery", "map_unlocks", "world_events",
    "inventory", "equipment", "skills", "appearance"};
}

std::string_view category_name(Category c) { return kNames[static_cast<std::size_t>(c)]; }

bool parse_category(std::string_view name, Category& out) {
    for (std::size_t i = 0; i < kNames.size(); ++i) {
        if (kNames[i] == name) {
            out = static_cast<Category>(i);
            return true;
        }
    }
    return false;
}

ProgressionRules ProgressionRules::defaults() {
    ProgressionRules r;
    for (Category c : {Category::Story, Category::SideQuests, Category::RegionDiscovery, Category::MapUnlocks,
                       Category::WorldEvents})
        r.policy[static_cast<std::size_t>(c)] = Policy::Shared;
    for (Category c : {Category::Inventory, Category::Equipment, Category::Skills, Category::Appearance})
        r.policy[static_cast<std::size_t>(c)] = Policy::Personal;
    return r;
}

SyncPlan plan_sync(const ProgressionRules& rules, const std::set<Flag>& host, const std::set<Flag>& guest) {
    SyncPlan plan;
    for (const Flag& f : host)
        if (rules.of(f.category) == Policy::Shared && !guest.contains(f)) plan.apply_to_guest.push_back(f);
    if (rules.guest_progress_counts_for_host)
        for (const Flag& f : guest)
            if (rules.of(f.category) == Policy::Shared && !host.contains(f)) plan.apply_to_host.push_back(f);
    return plan;
}

bool may_write_guest_save(const ProgressionRules& rules, bool guest_consented) {
    switch (rules.guest_save_mode) {
        case GuestSaveMode::SeparateSlot: return true;  // writes go to the co-op copy, never the original
        case GuestSaveMode::AskEachTime: return guest_consented;
        case GuestSaveMode::NeverWrite: return false;
    }
    return false;
}

}  // namespace cdcoop::sim
