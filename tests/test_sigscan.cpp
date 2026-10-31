#include "game/sigscan.hpp"
#include "test.hpp"

using namespace cdcoop::game;

TEST(sigscan_parse) {
    auto p = parse_pattern("48 8B 05 ?? ?? ? 90");
    CHECK(p && p->bytes.size() == 7 && p->mask[0] && !p->mask[3] && !p->mask[5] && p->bytes[6] == 0x90);
    CHECK(!parse_pattern("").has_value());
    CHECK(!parse_pattern("?? 48").has_value());  // must start with a concrete byte
    CHECK(!parse_pattern("4G").has_value());
    CHECK(!parse_pattern("488B").has_value());
}

TEST(sigscan_find_with_wildcards) {
    const std::vector<std::uint8_t> mem = {0x00, 0x48, 0x8B, 0x05, 0x11, 0x22, 0x33, 0x44, 0x48, 0x8B, 0x05, 0xAA};
    auto p = *parse_pattern("48 8B 05 ??");
    CHECK(find_first(mem, p) == 1);
    CHECK(find_first(mem, p, 2) == 8);
    CHECK(find_all(mem, p).size() == 2);
    CHECK(find_first(mem, *parse_pattern("48 8B 06")) == npos);
    CHECK(find_first(std::vector<std::uint8_t>{0x48}, p) == npos);  // haystack shorter than pattern
}

TEST(sigscan_rip_relative) {
    // mov rax, [rip+0x10] at offset 2: 48 8B 05 10 00 00 00 (7 bytes) -> target = 2 + 7 + 0x10 = 25
    std::vector<std::uint8_t> mem(64, 0x90);
    const std::uint8_t ins[] = {0x48, 0x8B, 0x05, 0x10, 0x00, 0x00, 0x00};
    std::copy(std::begin(ins), std::end(ins), mem.begin() + 2);
    CHECK(resolve_rip_relative(mem, 2, 3, 7) == std::size_t{25});
    CHECK(!resolve_rip_relative(mem, 62, 3, 7).has_value());
}
