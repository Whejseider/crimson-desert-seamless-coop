#pragma once
// Who gets what when loot drops or a chest is opened. Pure logic; the host decides, guests are told.
#include <cstdint>
#include <optional>
#include <vector>

namespace cdcoop::sim {

using PlayerId = std::uint64_t;  // SteamID64

enum class LootMode { Instanced, SharedPile, RoundRobin, NeedGreed };
enum class ItemKind { Normal, Currency, Quest };
enum class Roll { Need, Greed, Pass };

struct LootRules {
    LootMode mode = LootMode::Instanced;
    bool currency_to_all = true;     // currency goes to every nearby player
    bool quest_items_to_all = true;  // quest items go to every nearby player
    double nearby_radius_m = 60.0;
};

struct LootDrop {
    std::uint32_t item_id = 0;
    std::uint32_t quantity = 1;
    ItemKind kind = ItemKind::Normal;
};

struct Grant {
    PlayerId player = 0;
    std::uint32_t item_id = 0;
    std::uint32_t quantity = 0;
};

// State the host keeps across drops (round-robin turn).
struct LootState {
    std::size_t round_robin_next = 0;
};

struct RollEntry {
    PlayerId player = 0;
    Roll roll = Roll::Pass;
    std::uint32_t value = 0;  // 1..100, rolled by the host
};

// `nearby` = players eligible for this drop (in party order), `picker` = player who interacted with it.
// For NeedGreed, pass the collected rolls; nobody gets it if everyone passed.
std::vector<Grant> distribute(const LootRules& rules, LootState& state, const LootDrop& drop,
                              const std::vector<PlayerId>& nearby, PlayerId picker,
                              const std::vector<RollEntry>& rolls = {});

// Winner of a need/greed roll: any Need beats any Greed; highest value wins; ties go to the earliest entry.
std::optional<PlayerId> need_greed_winner(const std::vector<RollEntry>& rolls);

}  // namespace cdcoop::sim
