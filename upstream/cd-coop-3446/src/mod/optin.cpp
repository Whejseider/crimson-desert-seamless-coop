#include "mod/optin.h"

#include <windows.h>

#include <atomic>
#include <mutex>

#include "mod/log.h"
#include "mod/net/coop_worlds.h"
#include "mod/net/quest_advance.h"
#include "mod/net/quest_live.h"
#include "mod/save_io.h"

namespace cdmp::optin {
namespace {

std::mutex g_m;
std::wstring g_ini;
std::atomic<bool> g_available{false}, g_on{false}, g_card{false};
bool g_sniffer_ready = false, g_hooks = false, g_quest_hooks = false;

// The hooks and switches behind the feature; each only once.
void apply_on() {
    if (!g_hooks) {
        save_io::install(false);
        g_hooks = true;
    }
    net::worlds_configure(true, "");
    if (!g_quest_hooks && g_sniffer_ready && net::quest_live_enabled()) g_quest_hooks = net::quest_advance_install();
}

}  // namespace

void init(const std::wstring& ini_path, bool available, bool on, bool asked) {
    std::lock_guard lock(g_m);
    g_ini = ini_path;
    g_available = available;
    g_on = available && on;
    g_card = available && !asked;
    if (g_on) apply_on();
    log::info("Co-op+: experimental co-op saves + live quests {}{}", g_on ? "ON" : "off",
              g_card ? " (first launch: asking)" : "");
}

void set_sniffer_ready(bool ready) {
    std::lock_guard lock(g_m);
    g_sniffer_ready = ready;
    if (g_on) apply_on();
}

bool enabled() { return g_on.load(); }
bool card_visible() { return g_card.load(); }

void answer(bool on) {
    std::lock_guard lock(g_m);
    if (!g_available) return;
    g_card = false;
    WritePrivateProfileStringW(L"general", L"experimental_asked", L"1", g_ini.c_str());
    WritePrivateProfileStringW(L"coop", L"coop_worlds", on ? L"1" : L"0", g_ini.c_str());
    if (on && !g_on) {
        g_on = true;
        apply_on();
        log::info("Co-op+: experimental co-op saves + live quests turned ON");
    } else if (!on && g_on) {
        log::info("Co-op+: experimental co-op saves + live quests off from the next start");
    } else {
        log::info("Co-op+: experimental co-op saves + live quests: {}", on ? "on" : "not now");
    }
}

int panel_index() {
    if (!g_available) return 1;
    wchar_t v[8] = {};
    std::lock_guard lock(g_m);
    GetPrivateProfileStringW(L"coop", L"coop_worlds", L"0", v, 8, g_ini.c_str());
    return v[0] == L'1' ? 0 : 1;
}

}  // namespace cdmp::optin
