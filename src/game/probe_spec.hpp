#pragma once
// The M0 probe is data-driven: config/probe.toml lists signatures and pointer chains to read, so offsets can be
// found and corrected in Phase B without rebuilding the DLL. This file parses that config (pure).
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "game/sigscan.hpp"

namespace cdcoop::game {

enum class ValueType { F32, F32x3, I32, U32, F64, Ptr };

struct SignatureSpec {
    std::string name;
    std::string pattern_text;
    Pattern pattern;
    // If set, the signature points at an instruction with a RIP-relative operand; resolve it to get the address.
    bool rip_relative = false;
    std::size_t disp_offset = 0;
    std::size_t instr_len = 0;
    std::int64_t add = 0;  // added after resolving
};

struct ChainSpec {
    std::string name;       // e.g. "player.position"
    std::string base;       // a SignatureSpec name, or "module" for the game module base
    std::vector<std::int64_t> offsets;  // dereference after each offset except the last
    ValueType type = ValueType::F32;
};

struct ProbeSpec {
    double interval_s = 2.0;
    double duration_s = 120.0;
    std::vector<SignatureSpec> signatures;
    std::vector<ChainSpec> chains;
    std::vector<std::string> errors;
};

ProbeSpec parse_probe_spec(std::string_view toml_text);
std::string_view value_type_name(ValueType t);

}  // namespace cdcoop::game
