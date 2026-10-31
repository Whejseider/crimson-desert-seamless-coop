#include "game/probe_spec.hpp"

#include <toml.hpp>

namespace cdcoop::game {

std::string_view value_type_name(ValueType t) {
    switch (t) {
        case ValueType::F32: return "f32";
        case ValueType::F32x3: return "f32x3";
        case ValueType::I32: return "i32";
        case ValueType::U32: return "u32";
        case ValueType::F64: return "f64";
        case ValueType::Ptr: return "ptr";
    }
    return "?";
}

ProbeSpec parse_probe_spec(std::string_view text) {
    ProbeSpec spec;
    toml::parse_result res = toml::parse(text, std::string_view{"probe.toml"});
    if (!res) {
        spec.errors.push_back("probe.toml: line " + std::to_string(res.error().source().begin.line) + ": " +
                              std::string(res.error().description()));
        return spec;
    }
    const toml::table& t = res.table();
    spec.interval_s = t["interval_s"].value_or(spec.interval_s);
    spec.duration_s = t["duration_s"].value_or(spec.duration_s);

    if (auto arr = t["signature"].as_array()) {
        for (const auto& node : *arr) {
            const toml::table* s = node.as_table();
            if (!s) continue;
            SignatureSpec sig;
            sig.name = (*s)["name"].value_or(std::string{});
            sig.pattern_text = (*s)["pattern"].value_or(std::string{});
            auto pat = parse_pattern(sig.pattern_text);
            if (sig.name.empty() || !pat) {
                spec.errors.push_back("signature '" + sig.name + "': needs a name and a valid pattern");
                continue;
            }
            sig.pattern = *pat;
            if (auto rip = (*s)["rip"].as_table()) {
                sig.rip_relative = true;
                sig.disp_offset = static_cast<std::size_t>((*rip)["disp_offset"].value_or(int64_t{3}));
                sig.instr_len = static_cast<std::size_t>((*rip)["instr_len"].value_or(int64_t{7}));
            }
            sig.add = (*s)["add"].value_or(int64_t{0});
            spec.signatures.push_back(std::move(sig));
        }
    }
    if (auto arr = t["chain"].as_array()) {
        for (const auto& node : *arr) {
            const toml::table* c = node.as_table();
            if (!c) continue;
            ChainSpec ch;
            ch.name = (*c)["name"].value_or(std::string{});
            ch.base = (*c)["base"].value_or(std::string{"module"});
            if (auto offs = (*c)["offsets"].as_array())
                for (const auto& o : *offs) ch.offsets.push_back(o.value_or(int64_t{0}));
            const std::string type = (*c)["type"].value_or(std::string{"f32"});
            if (type == "f32") ch.type = ValueType::F32;
            else if (type == "f32x3") ch.type = ValueType::F32x3;
            else if (type == "i32") ch.type = ValueType::I32;
            else if (type == "u32") ch.type = ValueType::U32;
            else if (type == "f64") ch.type = ValueType::F64;
            else if (type == "ptr") ch.type = ValueType::Ptr;
            else { spec.errors.push_back("chain '" + ch.name + "': unknown type " + type); continue; }
            if (ch.name.empty() || ch.offsets.empty()) {
                spec.errors.push_back("chain '" + ch.name + "': needs a name and at least one offset");
                continue;
            }
            spec.chains.push_back(std::move(ch));
        }
    }
    return spec;
}

}  // namespace cdcoop::game
