#include "mod/net/coop_worlds.h"

#include <windows.h>

#include <atomic>
#include <cctype>
#include <format>
#include <mutex>
#include <thread>

#include "mod/log.h"
#include "mod/save_io.h"
#include "mod/net/troc_sniffer.h"
#include "mod/net/story_sync.h"

namespace cdmp::net {
namespace {

constexpr uint64_t kJoinStableMs = 8000;      // connected this long before we switch into the world
constexpr uint64_t kLeaveStableMs = 45000;    // the friend gone this long before we switch back (reconnects stay)
constexpr uint64_t kCountdownMs = 10000;
constexpr ULONG_PTR kOurInput = 0xC0C0A7;     // dwExtraInfo of keys/clicks we send (coop_activity's marker)
// The pause menu at 1920x1080 (2026-11-04, 1.0.0.3527): the Others tab, then Title Screen in its Options page.
constexpr int kOthersX = 804, kOthersY = 50, kTitleX = 466, kTitleY = 724;

std::mutex g_m;
bool g_enabled = false;
std::string g_test_partner;
WorldsInput g_in;
std::atomic<bool> g_in_world{false}, g_menu_open{false}, g_host{false};
uint64_t g_connected_since = 0, g_gone_since = 0;
std::string g_partner_key, g_partner_name;  // the last partner seen (kept while they are briefly away)
std::wstring g_want;                        // the world we should be in
uint64_t g_countdown_until = 0;
std::atomic<bool> g_switching{false};
std::string g_status, g_toast;

std::string narrow(const std::wstring& w) {
    std::string s;
    for (wchar_t c : w) s += static_cast<char>(c < 128 ? c : '?');
    return s;
}

std::wstring world_for(const std::string& key) {
    std::wstring w = L"with-";
    for (char c : key) w += std::isalnum(static_cast<unsigned char>(c)) || c == '-' ? static_cast<wchar_t>(c) : L'_';
    return w;
}

bool game_in_front() {
    DWORD pid = 0;
    GetWindowThreadProcessId(GetForegroundWindow(), &pid);
    return pid == GetCurrentProcessId();
}

void key(WORD vk, DWORD hold_ms) {
    const UINT scan = MapVirtualKeyW(vk, MAPVK_VK_TO_VSC);
    keybd_event(static_cast<BYTE>(vk), static_cast<BYTE>(scan), 0, kOurInput);
    Sleep(hold_ms);
    keybd_event(static_cast<BYTE>(vk), static_cast<BYTE>(scan), KEYEVENTF_KEYUP, kOurInput);
}

// A click at a 1920x1080 menu position, scaled to the game window (the UI scales with the height and stays centred).
bool click(int x, int y) {
    const HWND w = GetForegroundWindow();
    RECT rc{};
    if (!game_in_front() || !GetClientRect(w, &rc) || rc.bottom <= 0) return false;
    const double s = rc.bottom / 1080.0;
    POINT p{static_cast<LONG>(rc.right / 2.0 + (x - 960) * s), static_cast<LONG>(y * s)};
    ClientToScreen(w, &p);
    SetCursorPos(p.x, p.y);
    Sleep(80);
    mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, kOurInput);
    Sleep(60);
    mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, kOurInput);
    return true;
}

void set_status(const std::string& s) {
    std::lock_guard lock(g_m);
    g_status = s;
}

void toast(const std::string& s) {
    std::lock_guard lock(g_m);
    g_toast = "Co-op+\n" + s;
}

// Waits for `cond` up to timeout_ms (polling); the game window must be in front for keys, which the caller checks.
template <class F>
bool wait_for(F cond, uint64_t timeout_ms) {
    for (uint64_t t = 0; t < timeout_ms; t += 100) {
        if (cond()) return true;
        Sleep(100);
    }
    return cond();
}

bool wait_front(const std::string& what) {
    if (game_in_front()) return true;
    set_status(what + ": click the Crimson Desert window to continue.");
    return wait_for(game_in_front, 180000);
}

// The switch itself (its own thread): title screen, change the saves, Play.
void run_switch(std::wstring target, std::string partner_name) {
    const bool into = !target.empty();
    const std::string label = into ? "your co-op world with " + partner_name : "your own saves";
    const auto fail = [&](const std::string& why) {
        log::warn("Co-op worlds: switch to {} stopped: {}", narrow(target.empty() ? L"own saves" : target), why);
        set_status("Could not switch to " + label + ": " + why);
        toast("Could not switch to " + label + ": " + why);
        g_switching = false;
    };
    log::info("Co-op worlds: switching to {} ({})", narrow(target.empty() ? L"own saves" : target), label);
    set_status("Switching to " + label + "...");
    if (!wait_front("Switching to " + label)) return fail("the game window was not in front");
    key(VK_ESCAPE, 40);
    if (!wait_for([] { return g_menu_open.load(); }, 4000)) {
        key(VK_ESCAPE, 40);
        if (!wait_for([] { return g_menu_open.load(); }, 4000)) return fail("the menu did not open");
    }
    Sleep(800);
    if (!click(kOthersX, kOthersY)) return fail("the game window lost focus");
    Sleep(1200);
    if (!click(kTitleX, kTitleY)) return fail("the game window lost focus");
    Sleep(1200);
    if (!game_in_front()) return fail("the game window lost focus");
    key(VK_SPACE, 1500);  // "Return to the title screen?" Confirm (hold)
    // At the title: our character is gone for a while.
    if (!wait_for([] { return !g_in_world.load(); }, 30000)) return fail("the game did not go back to the title");
    Sleep(8000);
    std::string why;
    if (!save_io::set_world(target, &why)) {
        log::warn("Co-op worlds: {}; staying with the saves we had", why);
        toast("Could not open " + label + " (" + why + "); loading the saves you had.");
    } else if (into && !g_host) {
        // The guest's world follows the host's story: the host's quests go into the WORLD copy only.
        if (!story_put_into_world(save_io::world_folder(target), &why)) {
            log::warn("Co-op worlds: the host's story was not put into the world: {}", why);
            toast("Your co-op world keeps its own story this time (" + why + ").");
        }
    }
    // Play (E): the newest save of the saves now in use. The title takes a while to accept keys after the return
    // (a press 8 s in was ignored, 2026-11-04): press, then again every 12 s until the game starts loading.
    bool loading = false;
    for (int attempt = 0; attempt < 8 && !loading; ++attempt) {
        if (!wait_front("Loading " + label)) return fail("the game window was not in front (you are at the title: press E)");
        key('E', 80);
        loading = wait_for([] { return sniffer_world_loading() || g_in_world.load(); }, 12000);
    }
    if (!loading) return fail("the title screen did not start the game (press E)");
    if (!wait_for([] { return g_in_world.load(); }, 240000)) return fail("the save did not finish loading");
    log::info("Co-op worlds: now in {}", narrow(save_io::world().empty() ? L"own saves" : save_io::world()));
    set_status(save_io::world().empty() ? std::string() : "You are in " + label + ". Your own saves are untouched.");
    toast(save_io::world().empty() ? "Back in your own world (your own saves)."
                                   : "You are in " + label + ". Your own saves are untouched.");
    g_switching = false;
}

}  // namespace

void worlds_configure(bool enabled, const std::string& test_partner) {
    std::lock_guard lock(g_m);
    g_enabled = enabled;
    g_test_partner = test_partner;
    log::info("Co-op worlds: {}{}", enabled ? "on (a save per friend; your own saves only when you play alone)" : "off",
              test_partner.empty() ? "" : " | [dev] partner " + test_partner);
}

void worlds_tick(const WorldsInput& in_raw) {
    WorldsInput in = in_raw;
    g_in_world = in.in_world;
    g_menu_open = in.menu_open;
    g_host = in.host;
    std::lock_guard lock(g_m);
    if (!g_enabled) return;
    if (!g_test_partner.empty() && in.connected) in.partner_key = g_test_partner;
    if (in.connected && !in.partner_key.empty()) {
        if (!g_connected_since) {
            g_connected_since = in.now_ms ? in.now_ms : 1;
            if (!in.host) story_auto_request();  // the host's story, for our co-op world with them
        }
        g_gone_since = 0;
        g_partner_key = in.partner_key;
        g_partner_name = in.partner_name.empty() ? "your friend" : in.partner_name;
        if (in.now_ms - g_connected_since >= kJoinStableMs) g_want = world_for(g_partner_key);
    } else {
        g_connected_since = 0;
        if (!g_gone_since) g_gone_since = in.now_ms ? in.now_ms : 1;
        if (in.now_ms - g_gone_since >= kLeaveStableMs) g_want.clear();
    }
    if (g_switching) return;
    const std::wstring active = save_io::world();
    if (g_want == active) {
        g_countdown_until = 0;
        return;
    }
    // Only from the world itself (not a menu, not a loading screen), after a countdown the player sees.
    if (!in.in_world || in.menu_open) return;
    const std::string label = g_want.empty() ? "your own saves" : "your co-op world with " + g_partner_name;
    if (!g_countdown_until) {
        g_countdown_until = in.now_ms + kCountdownMs;
        g_toast = "Co-op+\nIn 10 s: back to the title screen, then into " + label + " (one loading screen). "
                  "Your own saves are never changed.";
        log::info("Co-op worlds: switch to {} in 10 s", narrow(g_want.empty() ? L"own saves" : g_want));
    }
    if (in.now_ms < g_countdown_until) {
        g_status = std::format("Switching to {} in {} s", label, (g_countdown_until - in.now_ms + 999) / 1000);
        return;
    }
    g_countdown_until = 0;
    g_switching = true;
    std::thread(run_switch, g_want, g_partner_name).detach();
}

std::string worlds_hud_text() {
    std::lock_guard lock(g_m);
    return g_status;
}

std::string worlds_take_toast() {
    std::lock_guard lock(g_m);
    std::string t;
    t.swap(g_toast);
    return t;
}

std::wstring worlds_active() { return save_io::world(); }

}  // namespace cdmp::net
