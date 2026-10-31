#include "sim/loot.hpp"
#include "test.hpp"

using namespace cdcoop::sim;

TEST(loot_instanced_everyone_gets_a_copy) {
    LootRules r; LootState s;
    const auto g = distribute(r, s, {42, 3, ItemKind::Normal}, {1, 2, 3}, 2);
    CHECK(g.size() == 3);
    for (const auto& x : g) CHECK(x.quantity == 3 && x.item_id == 42);
}

TEST(loot_picker_always_eligible) {
    LootRules r; LootState s;
    const auto g = distribute(r, s, {42, 1, ItemKind::Normal}, {1, 2}, 9);
    CHECK(g.size() == 3);
    CHECK(g.back().player == 9);
}

TEST(loot_shared_pile_goes_to_picker) {
    LootRules r; r.mode = LootMode::SharedPile; LootState s;
    const auto g = distribute(r, s, {42, 1, ItemKind::Normal}, {1, 2, 3}, 2);
    CHECK(g.size() == 1 && g[0].player == 2);
}

TEST(loot_currency_and_quest_items_go_to_all_even_in_shared_mode) {
    LootRules r; r.mode = LootMode::SharedPile; LootState s;
    CHECK(distribute(r, s, {7, 100, ItemKind::Currency}, {1, 2, 3}, 1).size() == 3);
    CHECK(distribute(r, s, {8, 1, ItemKind::Quest}, {1, 2, 3}, 1).size() == 3);
    r.currency_to_all = false;
    CHECK(distribute(r, s, {7, 100, ItemKind::Currency}, {1, 2, 3}, 1).size() == 1);
}

TEST(loot_round_robin_rotates) {
    LootRules r; r.mode = LootMode::RoundRobin; LootState s;
    std::vector<PlayerId> party = {1, 2, 3};
    CHECK(distribute(r, s, {1, 1, ItemKind::Normal}, party, 3)[0].player == 1);
    CHECK(distribute(r, s, {1, 1, ItemKind::Normal}, party, 3)[0].player == 2);
    CHECK(distribute(r, s, {1, 1, ItemKind::Normal}, party, 3)[0].player == 3);
    CHECK(distribute(r, s, {1, 1, ItemKind::Normal}, party, 3)[0].player == 1);
}

TEST(loot_need_beats_greed_and_ties_go_first) {
    CHECK(need_greed_winner({{1, Roll::Greed, 99}, {2, Roll::Need, 5}}) == PlayerId{2});
    CHECK(need_greed_winner({{1, Roll::Need, 50}, {2, Roll::Need, 50}}) == PlayerId{1});
    CHECK(need_greed_winner({{1, Roll::Greed, 10}, {2, Roll::Greed, 70}}) == PlayerId{2});
    CHECK(!need_greed_winner({{1, Roll::Pass, 99}, {2, Roll::Pass, 1}}).has_value());
}

TEST(loot_need_greed_all_pass_gives_nothing) {
    LootRules r; r.mode = LootMode::NeedGreed; LootState s;
    CHECK(distribute(r, s, {1, 1, ItemKind::Normal}, {1, 2}, 1, {{1, Roll::Pass, 3}, {2, Roll::Pass, 4}}).empty());
}
