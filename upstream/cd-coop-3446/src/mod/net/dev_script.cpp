#include "mod/net/dev_script.h"
#include "mod/input/pad_input.h"

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
#include "mod/net/time_sync.h"
#include "mod/net/dev_watch.h"
#include "mod/net/dev_memscan.h"
#include "mod/net/quest_events.h"
#include "mod/net/coop_hunt.h"
#include "mod/net/enemy_relay.h"
#include "mod/net/troc_sniffer.h"

namespace cdmp::net {
namespace {

struct Step {
    enum Kind { C2S, S2C, Wait, Teleport, Poke, Time, Watch, Day, Pad, KillGame, KillAll, MemScan, MemDiff, MemWatch, Advance, FScan, FFilter, FWatch, WaitFile, DScan, DFilter, DWatch, GSnap, GDiff, PtrScan, WatchStatus, Key, ClockScan } kind;
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
        if (kind == "watch") {
            std::string a;
            int len = 4, hits = 8;
            in >> a >> len >> hits;
            st.kind = Step::Watch;
            st.addr = static_cast<uintptr_t>(std::stoull(a, nullptr, 16));
            st.to[0] = static_cast<float>(len ? len : 4);
            st.to[1] = static_cast<float>(hits ? hits : 8);
            steps.push_back(st);
            continue;
        }
        if (kind == "pad") {  // pad <hex buttons (input/pad_input.h)> <ms>: a simulated pad press
            std::string b;
            in >> b >> st.wait_ms;
            st.kind = Step::Pad;
            st.addr = static_cast<uintptr_t>(std::stoul(b, nullptr, 16));
            steps.push_back(st);
            continue;
        }
        if (kind == "killgame") {  // killgame: KillCheatReq on a loaded wild animal (coop_hunt.h)
            st.kind = Step::KillGame;
            steps.push_back(st);
            continue;
        }
        if (kind == "memscan") {  // memscan <hex pattern>: find it in read/write memory (dev_memscan.h)
            std::string h;
            in >> h;
            st.kind = Step::MemScan;
            if (!parse_hex(h, st.payload)) continue;
            steps.push_back(st);
            continue;
        }
        if (kind == "advance") {  // advance <hex mission key>: the quest server advances it (quest_events.h)
            std::string k;
            in >> k;
            st.kind = Step::Advance;
            st.addr = static_cast<uintptr_t>(std::stoul(k, nullptr, 16));
            steps.push_back(st);
            continue;
        }
        if (kind == "waitfile") {  // waitfile <path>: wait (up to 5 min) until a test driver creates it, then delete it
            std::string f;
            std::getline(in >> std::ws, f);
            st.kind = Step::WaitFile;
            st.payload.assign(f.begin(), f.end());
            steps.push_back(st);
            continue;
        }
        if (kind == "dscan" || kind == "dfilter") {  // dscan / dfilter <lo> <hi>: the same for doubles
            in >> st.to[0] >> st.to[1];
            st.kind = kind == "dscan" ? Step::DScan : Step::DFilter;
            steps.push_back(st);
            continue;
        }
        if (kind == "dwatch") {
            in >> st.wait_ms;
            st.kind = Step::DWatch;
            steps.push_back(st);
            continue;
        }
        if (kind == "fscan") {  // fscan <float> [hi]: every float equal to it, or within [float, hi] (dev_memscan.h)
            in >> st.to[0];
            if (!(in >> st.to[1])) st.to[1] = st.to[0];
            st.kind = Step::FScan;
            steps.push_back(st);
            continue;
        }
        if (kind == "ffilter") {  // ffilter <lo> <hi>: keep those now within [lo, hi]
            in >> st.to[0] >> st.to[1];
            st.kind = Step::FFilter;
            steps.push_back(st);
            continue;
        }
        if (kind == "fwatch") {  // fwatch <hits>: hardware write watch on the first four left
            in >> st.wait_ms;
            st.kind = Step::FWatch;
            steps.push_back(st);
            continue;
        }
        if (kind == "key") {  // key <hex vk> <hold ms>: a key press from inside the game (as coop_worlds.cpp sends)
            std::string a;
            in >> a >> st.wait_ms;
            st.kind = Step::Key;
            st.addr = static_cast<uintptr_t>(std::stoull(a, nullptr, 16));
            steps.push_back(st);
            continue;
        }
        if (kind == "watchstatus") {  // watchstatus <hex offset> <len> <hits>: our status component (dev_memscan.h)
            std::string a;
            int len = 1, hits = 6;
            in >> a >> len >> hits;
            st.kind = Step::WatchStatus;
            st.addr = static_cast<uintptr_t>(std::stoull(a, nullptr, 16));
            st.to[0] = static_cast<float>(len ? len : 1);
            st.to[1] = static_cast<float>(hits ? hits : 6);
            steps.push_back(st);
            continue;
        }
        if (kind == "ptrscan") {  // ptrscan <hex actor key>: globals pointing at that actor (dev_memscan.h)
            std::string a;
            in >> a;
            st.kind = Step::PtrScan;
            st.addr = static_cast<uintptr_t>(std::stoull(a, nullptr, 16));
            steps.push_back(st);
            continue;
        }
        if (kind == "clockscan") {  // clockscan: values that run like the game's clock (dev_memscan.h)
            st.kind = Step::ClockScan;
            steps.push_back(st);
            continue;
        }
        if (kind == "gsnap" || kind == "gdiff") {  // gsnap / gdiff N: the exe's .data, before / after (dev_memscan.h)
            st.kind = kind == "gsnap" ? Step::GSnap : Step::GDiff;
            in >> st.wait_ms;
            steps.push_back(st);
            continue;
        }
        if (kind == "memdiff") {  // memdiff: which bytes around the memscan matches changed since
            st.kind = Step::MemDiff;
            steps.push_back(st);
            continue;
        }
        if (kind == "memwatch") {  // memwatch <rel> <hex value> <len> <hits> [nth]: hardware watch on a match
            std::string v;
            int rel = 0, len = 1, hits = 4, nth = 0;
            in >> rel >> v >> len >> hits >> nth;
            if (!parse_hex(v, st.payload)) continue;
            st.kind = Step::MemWatch;
            st.to[0] = static_cast<float>(rel);
            st.to[1] = static_cast<float>(len);
            st.to[2] = static_cast<float>(hits);
            st.wait_ms = static_cast<uint32_t>(nth);
            steps.push_back(st);
            continue;
        }
        if (kind == "killall") {  // killall: KillCheatReq on every loaded actor but animals (live quest research)
            st.kind = Step::KillAll;
            steps.push_back(st);
            continue;
        }
        if (kind == "day") {
            st.kind = Step::Day;
            in >> st.to[0];
            steps.push_back(st);
            continue;
        }
        if (kind == "time") {
            char colon = 0;
            if (!(in >> st.to[0] >> colon >> st.to[1]) || colon != ':') {
                log::warn("Dev script: line {} needs time hh:mm", lineno);
                continue;
            }
            st.kind = Step::Time;
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
        if (kind == "tp" || kind == "tprel") {  // tprel dx dy dz: from where we stood when the script started
            st.kind = Step::Teleport;
            if (!(in >> st.to[0] >> st.to[1] >> st.to[2])) {
                log::warn("Dev script: line {} needs {} x y z", lineno, kind);
                continue;
            }
            if (kind == "tprel") {
                for (int i = 0; i < 3; ++i) st.to[i] += pos[i];
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
            } else if (st.kind == Step::Watch) {
                dev_watch_start(st.addr, static_cast<int>(st.to[0]), static_cast<int>(st.to[1]));
            } else if (st.kind == Step::Pad) {
                input::dev_pad_press(static_cast<uint16_t>(st.addr), st.wait_ms ? st.wait_ms : 150);
                log::info("Dev script: pad 0x{:04X} for {} ms", st.addr, st.wait_ms);
                Sleep(st.wait_ms ? st.wait_ms : 150);
            } else if (st.kind == Step::MemScan) {
                dev_memscan(st.payload);
            } else if (st.kind == Step::Advance) {
                quest_events_request_advance(static_cast<uint32_t>(st.addr));
                log::info("Dev script: advance {:08X}", st.addr);
            } else if (st.kind == Step::WaitFile) {
                const std::string f(st.payload.begin(), st.payload.end());
                int i = 0;
                for (; i < 3000 && GetFileAttributesA(f.c_str()) == INVALID_FILE_ATTRIBUTES; ++i) Sleep(100);
                DeleteFileA(f.c_str());
                log::info("Dev script: waitfile {} {}", f, i < 3000 ? "found" : "timed out");
            } else if (st.kind == Step::DScan) {
                dev_dscan(st.to[0], st.to[1]);
            } else if (st.kind == Step::DFilter) {
                dev_dfilter(st.to[0], st.to[1]);
            } else if (st.kind == Step::DWatch) {
                dev_dwatch(static_cast<int>(st.wait_ms ? st.wait_ms : 6));
            } else if (st.kind == Step::FScan) {
                dev_fscan_range(st.to[0], st.to[1]);
            } else if (st.kind == Step::FFilter) {
                dev_ffilter(st.to[0], st.to[1]);
            } else if (st.kind == Step::FWatch) {
                dev_fwatch(static_cast<int>(st.wait_ms ? st.wait_ms : 8));
            } else if (st.kind == Step::Key) {
                const auto vk = static_cast<BYTE>(st.addr);
                const auto scan = static_cast<BYTE>(MapVirtualKeyW(vk, MAPVK_VK_TO_VSC));
                keybd_event(vk, scan, 0, 0);
                Sleep(st.wait_ms ? st.wait_ms : 60);
                keybd_event(vk, scan, KEYEVENTF_KEYUP, 0);
                log::info("Dev script: key 0x{:02X} ({} ms)", vk, st.wait_ms);
            } else if (st.kind == Step::WatchStatus) {
                dev_watch_status(static_cast<int>(st.addr), static_cast<int>(st.to[0]), static_cast<int>(st.to[1]));
            } else if (st.kind == Step::PtrScan) {
                dev_ptrscan(static_cast<uint32_t>(st.addr));
            } else if (st.kind == Step::ClockScan) {
                dev_clockscan();
            } else if (st.kind == Step::GSnap) {
                dev_gsnap();
            } else if (st.kind == Step::GDiff) {
                dev_gdiff(static_cast<int>(st.wait_ms));
            } else if (st.kind == Step::MemDiff) {
                dev_memdiff();
            } else if (st.kind == Step::MemWatch) {
                dev_memwatch(static_cast<int>(st.to[0]), st.payload, static_cast<int>(st.to[1]),
                             static_cast<int>(st.to[2]), static_cast<int>(st.wait_ms));
            } else if (st.kind == Step::KillGame) {
                hunt_dev_kill();
            } else if (st.kind == Step::KillAll) {
                int sent = 0;
                for (const auto& [key, variant] : enemy_loaded_variants()) {
                    if (hunt_animal_name(variant) || (key & 0xFF000000) != 0xB0000000 || sent >= 40) continue;
                    std::vector<uint8_t> pay(8, 0);  // KillCheatReq: u32 key | u32 option (0 = at once)
                    std::memcpy(pay.data(), &key, 4);
                    sent += sniffer_inject_c2s(make_c2s(2210, pay)) ? 1 : 0;
                    log::info("Dev script: KillCheatReq on 0x{:08X} (variant {})", key, variant);
                    Sleep(150);
                }
                log::info("Dev script: killall sent {}", sent);
            } else if (st.kind == Step::Day) {
                time_sync_dev_add_days(static_cast<int>(st.to[0]));
            } else if (st.kind == Step::Time) {
                time_sync_dev_set(static_cast<int>(st.to[0]), static_cast<int>(st.to[1]));
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
