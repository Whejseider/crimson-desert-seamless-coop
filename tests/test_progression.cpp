#include "sim/progression.hpp"
#include "test.hpp"

using namespace cdcoop::sim;

TEST(progression_defaults_share_world_keep_character_personal) {
    const auto r = ProgressionRules::defaults();
    CHECK(r.of(Category::Story) == Policy::Shared);
    CHECK(r.of(Category::RegionDiscovery) == Policy::Shared);
    CHECK(r.of(Category::Inventory) == Policy::Personal);
    CHECK(r.of(Category::Appearance) == Policy::Personal);
}

TEST(progression_sync_is_union_of_shared_flags) {
    const auto r = ProgressionRules::defaults();
    std::set<Flag> host = {{Category::Story, 1}, {Category::Story, 2}, {Category::Inventory, 9}};
    std::set<Flag> guest = {{Category::Story, 1}, {Category::MapUnlocks, 5}, {Category::Skills, 3}};
    const auto plan = plan_sync(r, host, guest);
    CHECK(plan.apply_to_guest.size() == 1 && plan.apply_to_guest[0].id == 2);
    CHECK(plan.apply_to_host.size() == 1 && plan.apply_to_host[0].id == 5);
}

TEST(progression_guest_progress_can_be_turned_off_for_host) {
    auto r = ProgressionRules::defaults();
    r.guest_progress_counts_for_host = false;
    const auto plan = plan_sync(r, {}, {{Category::Story, 7}});
    CHECK(plan.apply_to_host.empty());
}

TEST(progression_guest_save_consent) {
    auto r = ProgressionRules::defaults();
    CHECK(may_write_guest_save(r, false));  // separate slot: original save never touched
    r.guest_save_mode = GuestSaveMode::AskEachTime;
    CHECK(!may_write_guest_save(r, false));
    CHECK(may_write_guest_save(r, true));
    r.guest_save_mode = GuestSaveMode::NeverWrite;
    CHECK(!may_write_guest_save(r, true));
}

TEST(progression_category_names_roundtrip) {
    for (int i = 0; i < static_cast<int>(Category::Count); ++i) {
        Category c;
        CHECK(parse_category(category_name(static_cast<Category>(i)), c) && c == static_cast<Category>(i));
    }
    Category c;
    CHECK(!parse_category("nonsense", c));
}
