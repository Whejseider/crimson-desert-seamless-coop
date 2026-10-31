#include "net/interpolation.hpp"
#include "test.hpp"

using namespace cdcoop::net;

TEST(interp_blends_between_snapshots) {
    InterpBuffer b(0.1);
    b.push({1.0, {0, 0, 0}, 0});
    b.push({1.1, {10, 0, 0}, 0});
    const auto s = b.sample(1.15);  // render time 1.05 -> halfway
    CHECK(s.has_value());
    CHECK_NEAR(s->pos[0], 5.0, 1e-4);
}

TEST(interp_holds_edges_and_drops_out_of_order) {
    InterpBuffer b(0.1);
    CHECK(!b.sample(0).has_value());
    b.push({1.0, {1, 2, 3}, 0});
    b.push({0.5, {9, 9, 9}, 0});  // older: dropped
    CHECK(b.size() == 1);
    CHECK_NEAR(b.sample(0.0)->pos[0], 1.0, 1e-6);
    CHECK_NEAR(b.sample(5.0)->pos[2], 3.0, 1e-6);
}

TEST(interp_yaw_takes_short_way_round) {
    const float a = 3.0f, bb = -3.0f;  // ~0.28 rad apart across +-pi
    const float mid = lerp_angle(a, bb, 0.5f);
    CHECK(std::fabs(std::fabs(mid) - 3.14159f) < 0.01f);
}
