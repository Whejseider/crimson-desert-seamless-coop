#include "mod/net/dev_script.h"

#include <windows.h>

#include <cctype>
#include <cstring>
#include <fstream>
#include <sstream>
#include <thread>
#include <vector>

#include "mod/log.h"
#include "mod/net/local_capture.h"
#include "mod/net/remote_puppet.h"
#include "mod/net/revive_prompt.h"
#include "mod/net/troc_sniffer.h"

namespace cdmp::net {
namespace {

struct Step {
    enum Kind { C2S, S2C, Wait, Teleport, Poke } kind;
    uintptr_t addr = 0;
    float to[3] = {};
    uint16_t op = 0;
    std::vector<uint8_t> payload;
    uint32_t wait_ms = 0;
};

std::string hex_of(const void* p, size_t n) {
    static const char* d = "0123456789abcdef";
    std::string s;
    for (size_t i = 0; i < n; ++i) {
        const uint8_t b = static_cast<const uint8_t*>(p)[i];
        s += d[b >> 4];
        s += d[b & 15];
    }
    return s;
}

void replace_all(std::string& s, const std::string& from, const std::string& to) {
    for (size_t at = s.find(from); at != std::string::npos; at = s.find(from, at + to.size())) s.replace(at, from.size(), to);
}

bool parse_hex(const std::string& in, std::vector<uint8_t>& out) {
    std::string h;
    for (char c : in)
        if (std::isxdigit(static_cast<unsigned char>(c))) h += c;
        else if (!std::isspace(static_cast<unsigned char>(c))) return false;
    if (h.size() % 2) return false;
    for (size_t i = 0; i < h.size(); i += 2) out.push_back(static_cast<uint8_t>(std::stoul(h.substr(i, 2), nullptr, 16)));
    return true;
}

std::vector<uint8_t> make_c2s(uint16_t op, const std::vector<uint8_t>& payload) {
    std::vector<uint8_t> pkt(5 + payload.size());
    const uint16_t len = static_cast<uint16_t>(payload.size());
    std::memcpy(pkt.data(), &op, 2);
    pkt[2] = 0;
    std::memcpy(pkt.data() + 3, &len, 2);
    if (!payload.empty()) std::memcpy(pkt.data() + 5, payload.data(), payload.size());
    return pkt;
}

}  // namespace

int dev_script_run(const std::string& path, uint32_t own_player, float x, float y, float z) {
    std::ifstream f(path);
    if (!f) {
        log::warn("Dev script: cannot read {}", path);
        return -1;
    }
    const uint32_t user = capture_own_user_key();
    const float pos[3] = {x, y, z};
    std::vector<Step> steps;
    std::string line;
    int lineno = 0;
    while (std::getline(f, line)) {
        ++lineno;
        if (const auto hash = line.find('#'); hash != std::string::npos) line.resize(hash);
        std::istringstream in(line);
        std::string kind;
        if (!(in >> kind)) continue;
        Step st{};
        if (kind == "wait") {
            in >> st.wait_ms;
            st.kind = Step::Wait;
            st.wait_ms = std::min<uint32_t>(st.wait_ms, 10000);
            steps.push_back(st);
            continue;
        }
        if (kind == "poke") {
            std::string a, rest;
            in >> a;
            std::getline(in, rest);
            st.kind = Step::Poke;
            st.addr = static_cast<uintptr_t>(std::stoull(a, nullptr, 16));
            if (!st.addr || !parse_hex(rest, st.payload) || st.payload.empty()) {
                log::warn("Dev script: line {} needs poke <hex address> <hex bytes>", lineno);
                continue;
            }
            steps.push_back(std::move(st));
            continue;
        }
        if (kind == "tp") {
            st.kind = Step::Teleport;
            if (!(in >> st.to[0] >> st.to[1] >> st.to[2])) {
                log::warn("Dev script: line {} needs tp x y z", lineno);
                continue;
            }
            steps.push_back(st);
            continue;
        }
        unsigned op = 0;
        if ((kind != "c2s" && kind != "s2c") || !(in >> op)) {
            log::warn("Dev script: line {} not understood", lineno);
            continue;
        }
        std::string rest;
        std::getline(in, rest);
        replace_all(rest, "{P}", hex_of(&own_player, 4));
        replace_all(rest, "{U}", hex_of(&user, 4));
        replace_all(rest, "{POS}", hex_of(pos, 12));
        if (!parse_hex(rest, st.payload)) {
            log::warn("Dev script: line {} has a bad hex payload", lineno);
            continue;
        }
        st.kind = kind == "c2s" ? Step::C2S : Step::S2C;
        st.op = static_cast<uint16_t>(op);
        steps.push_back(std::move(st));
    }
    int packets = 0;
    for (const auto& st : steps) packets += st.kind != Step::Wait;
    log::info("Dev script: {} packets from {}", packets, path);
    std::thread([steps = std::move(steps)] {
        for (const auto& st : steps) {
            if (st.kind == Step::Wait) {
                Sleep(st.wait_ms);
            } else if (st.kind == Step::Poke) {
                MEMORY_BASIC_INFORMATION mbi{};
                const bool ok = VirtualQuery(reinterpret_cast<void*>(st.addr), &mbi, sizeof mbi) &&
                                mbi.State == MEM_COMMIT && (mbi.Protect & (PAGE_READWRITE | PAGE_EXECUTE_READWRITE));
                if (ok) std::memcpy(reinterpret_cast<void*>(st.addr), st.payload.data(), st.payload.size());
                log::info("Dev script: poke 0x{:X} ({} bytes){}", st.addr, st.payload.size(), ok ? "" : " REFUSED (not writable)");
            } else if (st.kind == Step::Teleport) {
                revive_teleport_to(st.to[0], st.to[1], st.to[2]);
                log::info("Dev script: teleport to ({:.0f}, {:.0f}, {:.0f})", st.to[0], st.to[1], st.to[2]);
            } else if (st.kind == Step::C2S) {
                const bool ok = sniffer_inject_c2s(make_c2s(st.op, st.payload));
                log::info("Dev script: c2s {} ({} bytes){}", st.op, st.payload.size(), ok ? "" : " NOT sent");
            } else {
                puppet_inject_s2c_batch({{st.op, st.payload}});
                log::info("Dev script: s2c {} ({} bytes)", st.op, st.payload.size());
            }
        }
    }).detach();
    return packets;
}

}  // namespace cdmp::net
