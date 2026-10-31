#pragma once
// IDA-style byte signatures ("48 8B 05 ?? ?? ?? ?? 48 85 C0") and scanning over a memory range.
// Pure: the Windows side just hands in the game module's .text bytes. Offsets found are recorded in docs/offsets.md.
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

namespace cdcoop::game {

struct Pattern {
    std::vector<std::uint8_t> bytes;
    std::vector<bool> mask;  // true = must match
    bool empty() const { return bytes.empty(); }
};

// Returns nullopt on malformed input. "?" and "??" are wildcards.
std::optional<Pattern> parse_pattern(std::string_view text);

inline constexpr std::size_t npos = static_cast<std::size_t>(-1);

std::size_t find_first(std::span<const std::uint8_t> haystack, const Pattern& p, std::size_t start = 0);
std::vector<std::size_t> find_all(std::span<const std::uint8_t> haystack, const Pattern& p, std::size_t limit = 16);

// Resolve a RIP-relative operand: `instr_offset` = offset of the instruction, `disp_offset` = offset of the
// 32-bit displacement within it, `instr_len` = instruction length. Returns the target offset in the same space.
std::optional<std::size_t> resolve_rip_relative(std::span<const std::uint8_t> image, std::size_t instr_offset,
                                                std::size_t disp_offset, std::size_t instr_len);

}  // namespace cdcoop::game
