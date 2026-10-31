#include "game/probe_spec.hpp"
#include "test.hpp"

using namespace cdcoop::game;

TEST(probe_spec_parses_signatures_and_chains) {
    const auto s = parse_probe_spec(R"(
interval_s = 1.0
[[signature]]
name = "world"
pattern = "48 8B 05 ?? ?? ?? ?? 48 85 C0"
rip = { disp_offset = 3, instr_len = 7 }
[[chain]]
name = "player.position"
base = "world"
offsets = [0, 0x18, 0x90]
type = "f32x3"
)");
    CHECK(s.errors.empty());
    CHECK_NEAR(s.interval_s, 1.0, 1e-9);
    CHECK(s.signatures.size() == 1 && s.signatures[0].rip_relative && s.signatures[0].instr_len == 7);
    CHECK(s.chains.size() == 1 && s.chains[0].offsets.size() == 3 && s.chains[0].offsets[2] == 0x90 &&
          s.chains[0].type == ValueType::F32x3);
}

TEST(probe_spec_reports_bad_entries) {
    const auto s = parse_probe_spec(R"(
[[signature]]
name = "bad"
pattern = "zz"
[[chain]]
name = "x"
offsets = [1]
type = "quaternion"
)");
    CHECK(s.errors.size() == 2);
    CHECK(s.signatures.empty() && s.chains.empty());
}
