#include "mod/settings_menu.h"

#include <windows.h>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <format>
#include <functional>
#include <mutex>

#include "mod/input/actions.h"
#include "mod/log.h"
#include "mod/net/coop_revive.h"
#include "mod/net/guest_hits.h"
#include "mod/net/loot_twin.h"
#include "mod/net/party_scaling.h"
#include "mod/net/party_quests.h"
#include "mod/net/remote_puppet.h"
#include "mod/net/stage_cinema.h"
#include "mod/net/story_sync.h"
#include "mod/net/revive_prompt.h"
#include "mod/net/weather_sync.h"
#include "mod/optin.h"

namespace cdmp::settings {
namespace {

struct Item {
    const char* label;
    const wchar_t* section;
    const wchar_t* key;
    std::vector<std::string> shown;    // what the panel says
    std::vector<std::wstring> stored;  // what cdmp.ini gets
    int index = 0;
    bool live = true;                  // applied now (else at the next start)
    std::function<void(int)> apply;
    std::function<int()> current;      // Co-op+: the value lives elsewhere (no ini key: section nullptr)
};

std::mutex g_mutex;
std::wstring g_ini;
std::vector<Item> g_items;
int g_sel = 0;
std::atomic<bool> g_open{false};
std::atomic<bool> g_respawn_partner{false};
std::atomic<bool> g_net_hud{false};
std::atomic<bool> g_colourblind{false};
std::atomic<float> g_ring{20.0f};
float g_health = 1.5f, g_boss = 1.7f;

std::vector<std::string> on_off() { return {"on", "off"}; }
std::vector<std::wstring> one_zero() { return {L"1", L"0"}; }

// 1.0 .. 3.0 in tenths.
Item multiplier(const char* label, const wchar_t* key, float value, std::function<void(int)> apply) {
    Item it{label, L"scaling", key, {}, {}, 0, true, std::move(apply)};
    for (int i = 10; i <= 30; ++i) {
        it.shown.push_back(std::format("x{:.1f}", i / 10.0));
        it.stored.push_back(std::format(L"{:.1f}", i / 10.0));
    }
    it.index = std::clamp(static_cast<int>(std::lround(value * 10.0f)) - 10, 0, 20);
    return it;
}

void change(int delta) {
    std::lock_guard lock(g_mutex);
    if (g_items.empty()) return;
    Item& it = g_items[g_sel];
    const int n = static_cast<int>(it.shown.size());
    if (it.current) it.index = it.current();
    it.index = (it.index + delta % n + n) % n;
    if (it.section) WritePrivateProfileStringW(it.section, it.key, it.stored[it.index].c_str(), g_ini.c_str());
    if (it.live && it.apply) it.apply(it.index);
    log::info("Settings: {} = {}{}", it.label, it.shown[it.index], it.live ? "" : " (next start)");
}

}  // namespace

void init(const std::wstring& ini_path, const Config& cfg, float health_2p, float boss_health_2p) {
    std::lock_guard lock(g_mutex);
    g_ini = ini_path;
    g_health = health_2p;
    g_boss = boss_health_2p;
    g_respawn_partner = cfg.coop_respawn_at_partner;
    g_items.clear();
    {
        Item x{"EXPERIMENTAL: co-op saves + live quests", nullptr, nullptr, {"on", "off (on = a save per friend)"}, {},
               1, true, [](int i) { optin::answer(i == 0); }};
        x.current = [] { return optin::panel_index(); };
        g_items.push_back(std::move(x));
    }
    g_items.push_back({"Who can join", L"net", L"join", {"any Steam friend", "only friends I invite"},
                       {L"friends", L"invite"}, cfg.net_join_invite_only ? 1 : 0, false, nullptr});
    g_items.push_back({"Loot of shared kills", L"coop", L"loot_mode", {"each their own", "host only", "first come"},
                       {L"per_player", L"host_only", L"first_come"}, cfg.coop_loot_mode, true,
                       [](int i) { net::loot_set_mode(i); }});
    g_items.push_back({"Guest finishes the host's enemies", L"coop", L"guest_finishes", on_off(), one_zero(),
                       cfg.coop_guest_finishes ? 0 : 1, true, [](int i) { net::guest_hits_set_finishes(i == 0); }});
    g_items.push_back({"Revive each other", L"coop", L"teammate_revive", on_off(), one_zero(),
                       cfg.coop_teammate_revive ? 0 : 1, true,
                       [dev = cfg.dev_revive_test](int i) { net::revive_partner_set(i == 0, dev); }});
    g_items.push_back({"After a checkpoint revive", L"coop", L"respawn_at", {"stay at the checkpoint", "back to my friend"},
                       {L"checkpoint", L"partner"}, cfg.coop_respawn_at_partner ? 1 : 0, true,
                       [](int i) { g_respawn_partner = i == 1; }});
    g_items.push_back({"Wait for my friend if I die at a boss", L"coop", L"boss_death_wait", on_off(), one_zero(),
                       cfg.coop_boss_death_wait ? 0 : 1, true, [](int i) { net::revive_set_boss_wait(i == 0); }});
    g_items.push_back({"Weather follows the host", L"coop", L"weather_sync", on_off(), one_zero(),
                       cfg.coop_weather_sync ? 0 : 1, true, [](int i) { net::weather_set_enabled(i == 0); }});
    g_items.push_back({"Tougher enemies with a guest", L"scaling", L"enabled", on_off(), one_zero(),
                       cfg.scaling_enabled ? 0 : 1, true, [](int i) { net::scaling_set_enabled(i == 0); }});
    // T40: the 2-player multipliers.
    g_items.push_back(multiplier("  Enemy health with a guest", L"panel_health_2p", health_2p, [](int i) {
        g_health = (i + 10) / 10.0f;
        net::scaling_set_2p(g_health, g_boss);
    }));
    g_items.push_back(multiplier("  Boss health with a guest", L"panel_boss_health_2p", boss_health_2p, [](int i) {
        g_boss = (i + 10) / 10.0f;
        net::scaling_set_2p(g_health, g_boss);
    }));
    g_net_hud = GetPrivateProfileIntW(L"overlay", L"net_hud", 0, ini_path.c_str()) != 0;
    g_items.push_back({"Ping and packet loss on screen", L"overlay", L"net_hud", on_off(), one_zero(),
                       g_net_hud ? 0 : 1, true, [](int i) { g_net_hud = i == 0; }});
    // T39: story spoilers.
    {
        const bool guard = GetPrivateProfileIntW(L"coop", L"spoiler_guard", 0, ini_path.c_str()) != 0;
        net::cinema_set_spoiler_guard(guard);
        net::party_quests_set_spoiler_guard(guard);
        g_items.push_back({"My friend's story (scenes, quest names)", L"coop", L"spoiler_guard",
                           {"shown", "hidden: no spoilers"}, {L"0", L"1"}, guard ? 1 : 0, true, [](int i) {
                               net::cinema_set_spoiler_guard(i == 1);
                               net::party_quests_set_spoiler_guard(i == 1);
                           }});
    }
    // T42: the duel ground.
    {
        const int ring = GetPrivateProfileIntW(L"coop", L"duel_ring_m", 20, ini_path.c_str());
        const int idx = ring <= 12 ? 0 : ring >= 35 ? 2 : 1;
        constexpr float kRings[] = {12.0f, 20.0f, 35.0f};
        g_ring = kRings[idx];
        g_items.push_back({"Duel ground (when I place it)", L"coop", L"duel_ring_m", {"small, 12 m", "normal, 20 m", "large, 35 m"},
                           {L"12", L"20", L"35"}, idx, true, [](int i) {
                               constexpr float r[] = {12.0f, 20.0f, 35.0f};
                               g_ring = r[i];
                           }});
    }
    // T34: colours.
    g_colourblind = GetPrivateProfileIntW(L"overlay", L"colourblind", 0, ini_path.c_str()) != 0;
    // Co-op+ story sync (story_sync.h): the guest asks; the host decides whether it shares.
    {
        Item copy{"Copy my friend's story into my save (guest)", nullptr, nullptr, {"no", "yes"}, {}, 0, true,
                  [](int i) { net::story_set_wanted(i == 1); }};
        copy.current = [] { return net::story_wanted_index(); };
        g_items.push_back(std::move(copy));
    }
    g_items.push_back({"Let my guest copy my story (host)", L"coop", L"share_story", on_off(), one_zero(),
                       cfg.coop_share_story ? 0 : 1, true, [](int i) { net::story_set_share(i == 0); }});
    g_items.push_back({"Player colours", L"overlay", L"colourblind", {"gold host, blue guest", "colour-blind safe"},
                       {L"0", L"1"}, g_colourblind ? 1 : 0, true, [](int i) { g_colourblind = i == 1; }});
    {
        Item pin{"My friend's map pin colour", L"coop", L"map_pin_colour", {}, {}, cfg.coop_map_pin_colour, true,
                 [shape = cfg.coop_map_pin_shape](int i) { net::puppet_map_pin_style(shape, static_cast<uint8_t>(i)); }};
        for (int i = 0; i < 15; ++i) {
            pin.shown.push_back(i == 14 ? "15 (the game's)" : std::to_string(i + 1));
            pin.stored.push_back(std::to_wstring(i));
        }
        g_items.push_back(std::move(pin));
    }
    g_items.push_back({"Name over my friend (Ctrl+F8 too)", L"overlay", L"nameplates", on_off(), one_zero(),
                       cfg.overlay_nameplates ? 0 : 1, false, nullptr});
}

void set_panel_open(bool open) { g_open = open; }
bool panel_open() { return g_open.load(); }

bool hook_key(unsigned vk, bool down) {
    if (!g_open.load()) return false;
    if (vk != VK_UP && vk != VK_DOWN && vk != VK_LEFT && vk != VK_RIGHT) return false;
    if (down) step(vk == VK_LEFT ? -1 : vk == VK_RIGHT ? 1 : 0, vk == VK_UP ? -1 : vk == VK_DOWN ? 1 : 0);
    return true;
}

void step(int dx, int dy) {
    if (dy) {
        std::lock_guard lock(g_mutex);
        const int n = static_cast<int>(g_items.size());
        if (n) g_sel = (g_sel + dy % n + n) % n;
    }
    if (dx) change(dx);
}

std::vector<Line> lines() {
    std::vector<Line> out;
    std::lock_guard lock(g_mutex);
    out.push_back({"Settings: Up/Down choose, Left/Right change (pad: D-pad)", false});
    for (int i = 0; i < static_cast<int>(g_items.size()); ++i) {
        const Item& it = g_items[i];
        const int idx = it.current ? it.current() : it.index;
        out.push_back({std::format("{}{}: {}{}", i == g_sel ? "> " : "  ", it.label, it.shown[idx],
                                   it.live ? "" : "  (next start)"),
                       i == g_sel});
    }
    out.push_back({"Keys: change them under [keys] in cdmp.ini, e.g. Invite=Ctrl+F1 (README)", false});
    return out;
}

bool respawn_at_partner() { return g_respawn_partner.load(); }
bool net_hud() { return g_net_hud.load(); }
bool colourblind() { return g_colourblind.load(); }
float duel_ring_m() { return g_ring.load(); }

}  // namespace cdmp::settings
