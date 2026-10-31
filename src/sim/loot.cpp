#include "sim/loot.hpp"

#include <algorithm>

namespace cdcoop::sim {

std::optional<PlayerId> need_greed_winner(const std::vector<RollEntry>& rolls) {
    const RollEntry* best = nullptr;
    auto rank = [](Roll r) { return r == Roll::Need ? 2 : r == Roll::Greed ? 1 : 0; };
    for (const RollEntry& r : rolls) {
        if (r.roll == Roll::Pass) continue;
        if (!best || rank(r.roll) > rank(best->roll) || (rank(r.roll) == rank(best->roll) && r.value > best->value))
            best = &r;
    }
    if (!best) return std::nullopt;
    return best->player;
}

std::vector<Grant> distribute(const LootRules& rules, LootState& state, const LootDrop& drop,
                              const std::vector<PlayerId>& nearby, PlayerId picker,
                              const std::vector<RollEntry>& rolls) {
    std::vector<Grant> out;
    std::vector<PlayerId> eligible = nearby;
    if (std::find(eligible.begin(), eligible.end(), picker) == eligible.end()) eligible.push_back(picker);

    const bool to_all = (drop.kind == ItemKind::Currency && rules.currency_to_all) ||
                        (drop.kind == ItemKind::Quest && rules.quest_items_to_all);
    if (to_all || rules.mode == LootMode::Instanced) {
        for (PlayerId p : eligible) out.push_back({p, drop.item_id, drop.quantity});
        return out;
    }
    switch (rules.mode) {
        case LootMode::SharedPile:
            out.push_back({picker, drop.item_id, drop.quantity});
            break;
        case LootMode::RoundRobin: {
            const PlayerId p = eligible[state.round_robin_next % eligible.size()];
            state.round_robin_next = (state.round_robin_next + 1) % eligible.size();
            out.push_back({p, drop.item_id, drop.quantity});
            break;
        }
        case LootMode::NeedGreed:
            if (auto w = need_greed_winner(rolls)) out.push_back({*w, drop.item_id, drop.quantity});
            break;
        case LootMode::Instanced:
            break;  // handled above
    }
    return out;
}

}  // namespace cdcoop::sim
