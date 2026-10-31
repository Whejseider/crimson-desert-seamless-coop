#include "game/probe.hpp"

#include <windows.h>

#include <chrono>
#include <cstring>
#include <optional>
#include <span>
#include <cstdio>
#include <fstream>
#include <map>
#include <sstream>
#include <thread>

#include "core/log.hpp"
#include "core/safety.hpp"
#include "game/probe_spec.hpp"
#include "net/steam_runtime.hpp"

namespace cdcoop::game {
namespace {

struct Image {
    std::uintptr_t base = 0;
    std::span<const std::uint8_t> text;  // .text section
    std::uintptr_t text_va = 0;
};

Image game_image() {
    Image img;
    auto base = reinterpret_cast<std::uintptr_t>(GetModuleHandleW(nullptr));
    img.base = base;
    auto dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
    auto nt = reinterpret_cast<const IMAGE_NT_HEADERS64*>(base + dos->e_lfanew);
    auto sec = IMAGE_FIRST_SECTION(nt);
    for (WORD i = 0; i < nt->FileHeader.NumberOfSections; ++i, ++sec) {
        if (std::memcmp(sec->Name, ".text", 5) == 0) {
            img.text_va = base + sec->VirtualAddress;
            img.text = {reinterpret_cast<const std::uint8_t*>(img.text_va), sec->Misc.VirtualSize};
            break;
        }
    }
    return img;
}

std::string hex(std::uintptr_t v) { char b[32]; std::snprintf(b, sizeof b, "0x%llX", static_cast<unsigned long long>(v)); return b; }

std::string read_value(std::uintptr_t addr, ValueType t) {
    char b[96];
    switch (t) {
        case ValueType::F32: { float v; if (!safety::safe_read(addr, &v, 4)) return "<unreadable>"; std::snprintf(b, sizeof b, "%.3f", v); return b; }
        case ValueType::F32x3: { float v[3]; if (!safety::safe_read(addr, v, 12)) return "<unreadable>"; std::snprintf(b, sizeof b, "(%.2f, %.2f, %.2f)", v[0], v[1], v[2]); return b; }
        case ValueType::I32: { std::int32_t v; if (!safety::safe_read(addr, &v, 4)) return "<unreadable>"; return std::to_string(v); }
        case ValueType::U32: { std::uint32_t v; if (!safety::safe_read(addr, &v, 4)) return "<unreadable>"; return std::to_string(v); }
        case ValueType::F64: { double v; if (!safety::safe_read(addr, &v, 8)) return "<unreadable>"; std::snprintf(b, sizeof b, "%.4f", v); return b; }
        case ValueType::Ptr: { std::uintptr_t v; if (!safety::safe_read(addr, &v, 8)) return "<unreadable>"; return hex(v); }
    }
    return "?";
}

// base -> [+o0] -> [+o1] ... -> +oLast = address of value
std::optional<std::uintptr_t> follow(std::uintptr_t base, const std::vector<std::int64_t>& offs) {
    std::uintptr_t a = base;
    for (std::size_t i = 0; i < offs.size(); ++i) {
        a += static_cast<std::uintptr_t>(offs[i]);
        if (i + 1 == offs.size()) return a;
        std::uintptr_t next;
        if (!safety::safe_read(a, &next, sizeof next) || next == 0) return std::nullopt;
        a = next;
    }
    return a;
}

void run(ProbeSpec spec) {
    const Image img = game_image();
    log::info("Probe: game module base " + hex(img.base) + ", .text " + hex(img.text_va) + " size " + hex(img.text.size()));
    std::map<std::string, std::uintptr_t> found;
    found["module"] = img.base;
    for (const auto& s : spec.signatures) {
        const auto hits = find_all(img.text, s.pattern, 4);
        if (hits.empty()) { log::warn("Probe: signature '" + s.name + "' not found"); continue; }
        if (hits.size() > 1) log::warn("Probe: signature '" + s.name + "' matches " + std::to_string(hits.size()) + " places, using the first");
        std::uintptr_t addr = img.text_va + hits[0];
        if (s.rip_relative) {
            auto t = resolve_rip_relative(img.text, hits[0], s.disp_offset, s.instr_len);
            if (!t) { log::warn("Probe: signature '" + s.name + "' RIP operand out of range"); continue; }
            addr = img.text_va + *t;
        }
        addr += static_cast<std::uintptr_t>(s.add);
        found[s.name] = addr;
        log::info("Probe: " + s.name + " = " + hex(addr) + " (RVA " + hex(addr - img.base) + ")");
    }

    const auto start = std::chrono::steady_clock::now();
    bool steam_logged = false;
    while (safety::active()) {
        if (!steam_logged) {
            const auto st = net::query_steam();
            if (st.user_ready) {
                log::info("Steam: signed in as '" + st.persona_name + "' (" + std::to_string(st.steam_id) + "), lobbies API " +
                          (st.matchmaking_api ? "ok" : "MISSING") + ", messages API " + (st.messages_api ? "ok" : "MISSING"));
                if (!st.missing.empty()) log::warn("Steam: missing exports: " + st.missing);
                steam_logged = true;
            }
        }
        std::string line = "Probe:";
        for (const auto& c : spec.chains) {
            auto it = found.find(c.base);
            if (it == found.end()) { line += " " + c.name + "=<no base>"; continue; }
            auto addr = follow(it->second, c.offsets);
            line += " " + c.name + "=" + (addr ? read_value(*addr, c.type) : std::string("<null>"));
        }
        if (!spec.chains.empty()) log::info(line);
        const double elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
        if (elapsed > spec.duration_s) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(spec.interval_s * 1000)));
    }
    log::info("Probe: finished");
}

}  // namespace

bool start_probe(const std::filesystem::path& probe_toml) {
    std::ifstream in(probe_toml, std::ios::binary);
    if (!in) { log::warn("Probe: no probe.toml, skipping"); return false; }
    std::stringstream ss;
    ss << in.rdbuf();
    ProbeSpec spec = parse_probe_spec(ss.str());
    for (const auto& e : spec.errors) log::warn("Probe config: " + e);
    std::thread(run, std::move(spec)).detach();
    return true;
}

}  // namespace cdcoop::game
