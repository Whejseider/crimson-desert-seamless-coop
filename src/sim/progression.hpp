#pragma once
// What is shared between players and what stays personal, and how shared progress is merged.
// Pure logic: the game/ layer reads/writes the actual flags; this decides what to apply where.
#include <array>
#include <cstdint>
#include <set>
#include <string_view>
#include <vector>

namespace cdcoop::sim {

enum class Category : std::uint8_t {
    Story,            // main quest progress
    SideQuests,
    RegionDiscovery,  // discovered areas, fog of war
    MapUnlocks,       // fast travel points, abyss nexus, viewpoints
    WorldEvents,      // key world state (bosses killed, gates opened)
    Inventory,
    Equipment,
    Skills,
    Appearance,
    Count
};

enum class Policy : std::uint8_t { Shared, Personal };

std::string_view category_name(Category c);
bool parse_category(std::string_view name, Category& out);

enum class GuestSaveMode : std::uint8_t {
    SeparateSlot,  // guest plays on a co-op copy of their character; their single-player save is untouched (default)
    AskEachTime,   // guest is asked at session end whether to write shared progress into their own save
    NeverWrite,    // nothing from the session is written to the guest's save
};

struct ProgressionRules {
    std::array<Policy, static_cast<std::size_t>(Category::Count)> policy{};
    bool guest_progress_counts_for_host = true;  // guests completing shared objectives advance the host's world
    bool backup_saves_before_session = true;
    GuestSaveMode guest_save_mode = GuestSaveMode::SeparateSlot;

    Policy of(Category c) const { return policy[static_cast<std::size_t>(c)]; }
    static ProgressionRules defaults();
};

// A progress flag: a quest step, a discovered region, an unlocked waypoint... Ids come from the game.
struct Flag {
    Category category = Category::Story;
    std::uint32_t id = 0;
    auto operator<=>(const Flag&) const = default;
};

struct SyncPlan {
    std::vector<Flag> apply_to_guest;  // host has it, guest doesn't
    std::vector<Flag> apply_to_host;   // guest has it, host doesn't (only if guest_progress_counts_for_host)
};

// Shared flags are merged as a union (progress is never taken away). Personal categories are ignored.
SyncPlan plan_sync(const ProgressionRules& rules, const std::set<Flag>& host, const std::set<Flag>& guest);

// May the session write shared progress into this guest's own save right now?
bool may_write_guest_save(const ProgressionRules& rules, bool guest_consented);

}  // namespace cdcoop::sim
