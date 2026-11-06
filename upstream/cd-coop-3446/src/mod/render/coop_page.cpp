#include "mod/render/coop_page.h"

#include <windows.h>

#include <imgui.h>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <deque>
#include <format>
#include <functional>
#include <mutex>
#include <thread>

#include "mod/game/ui_state.h"
#include "mod/input/actions.h"
#include "mod/input/pad_input.h"
#include "mod/log.h"
#include "mod/render/game_ui.h"
#include "mod/render/hud.h"
#include "mod/settings_menu.h"
#include "mod/net/coop_lobby.h"

namespace cdmp::render {
namespace {

// The game's look (game_ui.cpp): warm gold, light text, dim text, thin light rules.
constexpr ImU32 kGold = IM_COL32(232, 200, 130, 255);
constexpr ImU32 kText = IM_COL32(232, 228, 220, 255);
constexpr ImU32 kDim = IM_COL32(170, 164, 152, 255);
constexpr ImU32 kRule = IM_COL32(205, 195, 175, 90);
constexpr ULONG_PTR kOurInput = 0xC0C0A7;  // input we send ourselves (coop_worlds / coop_activity mark it so)

const char* const kSections[] = {"Session", "Saves & Story", "Combat & Loot", "Reviving", "World",
                                 "Display", "Challenges", "Controls", "Status"};
constexpr int kSectionCount = static_cast<int>(std::size(kSections));

std::atomic<bool> g_open{false}, g_details{false};
std::atomic<bool> g_lobby_shown{false};  // the title lobby is on screen (its keys and clicks are ours)
int g_lobby_sel = 0;
std::atomic<HWND> g_hwnd{nullptr};

// Input that the hook threads take for the page, read by the render thread.
std::mutex g_in_mutex;
std::deque<unsigned> g_keys;
std::deque<POINT> g_clicks;  // screen coordinates
int g_wheel = 0;
// The tab's rectangle on screen while it is drawn (a click on it opens the page).
std::atomic<long> g_tab_l{0}, g_tab_t{0}, g_tab_r{0}, g_tab_b{0};
std::atomic<bool> g_tab_shown{false};
std::atomic<bool> g_swallow_up{false};

int g_section = 0, g_row = 0, g_top = 0;
bool g_focus_rows = false;  // keyboard / pad: Up / Down move in the rows (true) or the sections (false)

bool game_in_front() {
    DWORD pid = 0;
    GetWindowThreadProcessId(GetForegroundWindow(), &pid);
    return pid == GetCurrentProcessId();
}

LRESULT CALLBACK mouse_hook(int code, WPARAM wp, LPARAM lp) {
    if (code == HC_ACTION && game_in_front()) {
        const auto* m = reinterpret_cast<const MSLLHOOKSTRUCT*>(lp);
        if (m->dwExtraInfo != kOurInput) {
            const bool down = wp == WM_LBUTTONDOWN, up = wp == WM_LBUTTONUP;
            if (g_open.load() || g_lobby_shown.load()) {
                if (down || up || wp == WM_RBUTTONDOWN || wp == WM_RBUTTONUP || wp == WM_MOUSEWHEEL) {
                    std::lock_guard lock(g_in_mutex);
                    if (down && g_clicks.size() < 16) g_clicks.push_back(m->pt);
                    if (wp == WM_MOUSEWHEEL) g_wheel += static_cast<short>(HIWORD(m->mouseData)) / WHEEL_DELTA;
                    if (wp == WM_RBUTTONDOWN) g_keys.push_back(VK_ESCAPE);  // right click: back, as in the game
                    return 1;
                }
            } else if (down && g_tab_shown.load() && m->pt.x >= g_tab_l && m->pt.x <= g_tab_r && m->pt.y >= g_tab_t &&
                       m->pt.y <= g_tab_b) {
                std::lock_guard lock(g_in_mutex);
                g_clicks.push_back(m->pt);
                g_swallow_up = true;
                return 1;
            } else if (up && g_swallow_up.exchange(false)) {
                return 1;
            }
        }
    }
    return CallNextHookEx(nullptr, code, wp, lp);
}

void start_mouse_hook() {
    static std::once_flag once;
    std::call_once(once, [] {
        std::thread([] {
            if (!SetWindowsHookExW(WH_MOUSE_LL, mouse_hook, GetModuleHandleW(nullptr), 0)) {
                log::warn("Co-op page: mouse hook not installed ({}); the page works with keys and pads", GetLastError());
                return;
            }
            MSG m;
            while (GetMessageW(&m, nullptr, 0, 0) > 0) {
            }
        }).detach();
    });
}

// ---- the page's rows
struct Row {
    enum Kind { Setting, Button, Info } kind = Info;
    std::string label, value, help;
    bool live = true;
    std::function<void(int)> act;  // Setting: change by delta; Button: press (delta ignored)
};

std::string connection_text(const HudData& d) {
    if (!d.connected) return "not connected";
    std::string s = std::format("{:.0f} ms", d.rtt_ms);
    if (d.loss_pct >= 0) s += std::format(", {:.1f} % loss", d.loss_pct);
    return s;
}

Row button(const std::string& label, input::Action a, const std::string& help) {
    Row r{Row::Button, label, "{" + std::string(input::binding(a).name) + "}", help, true, nullptr};
    r.act = [a](int) { input::trigger(a); };
    return r;
}

std::vector<Row> rows_for(int section, const HudData& d, const std::vector<std::string>& status) {
    std::vector<Row> rows;
    const std::string name = kSections[section];
    if (name == "Session") {
        rows.push_back({Row::Info, "Playing with", d.connected ? d.peer_name : "nobody yet", "", true, nullptr});
        rows.push_back({Row::Info, "Connection", connection_text(d), "", true, nullptr});
        rows.push_back(button("Invite a friend", input::Action::Invite,
                              "Opens Steam's invite list: your friend accepts it and joins your world."));
        rows.push_back(button("Leave the session", input::Action::Leave,
                              "Ends the session: you go back to your own world, nothing of your friend stays."));
        rows.push_back(button("Go to my friend", input::Action::GoToFriend, "Takes you to your friend's side."));
        rows.push_back(button("Bring my friend to me", input::Action::Summon,
                              "Asks your friend to come to you; they accept or decline."));
        rows.push_back(button("Resynchronise", input::Action::Resync,
                              "Joins again from scratch when things drift apart (both characters are sent again)."));
    } else if (name == "Controls") {
        static const std::pair<const char*, input::Action> kList[] = {
            {"Co-op page", input::Action::Panel},        {"Invite a friend", input::Action::Invite},
            {"Leave", input::Action::Leave},             {"Ping / quick messages", input::Action::Ping},
            {"Go to my friend", input::Action::GoToFriend}, {"Bring my friend to me", input::Action::Summon},
            {"Challenge / accept", input::Action::Challenge}, {"Decline", input::Action::Decline},
            {"Revive my friend (hold)", input::Action::ReviveFriend}, {"Chat", input::Action::Chat},
            {"Name tags", input::Action::Nameplates},    {"Resynchronise", input::Action::Resync},
            {"Controls card", input::Action::Help}};
        for (const auto& [label, a] : kList) {
            rows.push_back({Row::Info, label, "{" + std::string(input::binding(a).name) + "}",
                            "Change keys under [keys] in cdmp.ini (README).", true, nullptr});
        }
    } else if (name == "Status") {
        Row det{Row::Setting, "Status panel (top left)", g_details.load() ? "shown" : "hidden",
                "The detailed connection panel, for reporting a problem.", true, nullptr};
        det.act = [](int) { g_details = !g_details.load(); };
        rows.push_back(det);
        for (const auto& s : status) rows.push_back({Row::Info, s, "", "", true, nullptr});
    } else {
        const auto items = settings::page_rows(name);
        for (int i = 0; i < static_cast<int>(items.size()); ++i) {
            Row r{Row::Setting, items[i].label, items[i].value, items[i].help, items[i].live, nullptr};
            r.act = [name, i](int delta) { settings::page_change(name, i, delta == 0 ? 1 : delta); };
            rows.push_back(std::move(r));
        }
    }
    return rows;
}

// ---- drawing helpers (1920x1080 positions; the game's menu scales with the height and stays centred)
struct Space {
    float w, h, s;
    float x(float v) const { return w * 0.5f + (v - 960.0f) * s; }
    float y(float v) const { return v * s; }
};

float text(ImDrawList* dl, float x, float y, float size, const std::string& t, ImU32 c, bool bold = false) {
    return ui::rich_text(dl, x, y, size, t, c, bold ? ui::serif_bold() : ui::serif());
}
float text_w(float size, const std::string& t, bool bold = false) {
    return ui::rich_text(nullptr, 0, 0, size, t, 0, bold ? ui::serif_bold() : ui::serif());
}

// A light key cap with a caption after it ("Esc Close"), right to left from `right`; returns the new right edge.
float hint(ImDrawList* dl, float right, float y, float size, const std::string& key, const std::string& caption) {
    const float cw = text_w(size, caption);
    const float kw = text_w(size * 0.85f, key, true) + size * 0.7f;
    const float h = size * 1.2f;
    text(dl, right - cw, y, size, caption, kText);
    const float kx = right - cw - size * 0.4f - kw;
    dl->AddRectFilled(ImVec2(kx, y - size * 0.1f), ImVec2(kx + kw, y - size * 0.1f + h), IM_COL32(214, 210, 202, 235),
                      size * 0.18f);
    text(dl, kx + size * 0.35f, y + size * 0.05f, size * 0.85f, key, IM_COL32(28, 26, 24, 255), true);
    return kx - size * 1.1f;
}

void wrap(ImDrawList* dl, float x, float y, float width, float size, const std::string& t, ImU32 c) {
    std::string line, word;
    float yy = y;
    auto flush = [&] {
        if (!line.empty()) text(dl, x, yy, size, line, c);
        yy += size * 1.3f;
        line.clear();
    };
    for (size_t i = 0; i <= t.size(); ++i) {
        if (i == t.size() || t[i] == ' ') {
            const std::string cand = line.empty() ? word : line + " " + word;
            if (!line.empty() && text_w(size, cand) > width) {
                flush();
                line = word;
            } else {
                line = cand;
            }
            word.clear();
        } else {
            word += t[i];
        }
    }
    flush();
}

bool inside(const ImVec2& p, float l, float t, float r, float b) { return p.x >= l && p.x <= r && p.y >= t && p.y <= b; }

// ---- the title lobby (net/coop_lobby.h): the co-op main menu before anyone plays
struct LobbyItem {
    std::string label;
    bool enabled = true;
    bool chosen = false;  // the host's choice (or the guest's ready), marked
    std::function<void()> act;
};

std::vector<LobbyItem> lobby_items(const net::LobbyView& v) {
    std::vector<LobbyItem> items;
    const bool counting = v.countdown >= 0;
    if (v.host) {
        items.push_back({"Invite Players", !counting, false, [] { input::trigger(input::Action::Invite); }});
        if (v.worlds_on) {
            items.push_back({"Start a New Save Together", v.connected && !counting, v.choice == 1, [] { net::lobby_new_save(); }});
            items.push_back({"Continue Our Save", v.connected && v.can_continue && !counting, v.choice == 2,
                             [] { net::lobby_continue(); }});
        }
        items.push_back({counting ? std::format("Starting in {}", v.countdown) : std::string("Start"),
                         v.connected && v.partner_ready && v.choice != 0 && v.save_ready && !counting, false,
                         [] { net::lobby_start(); }});
    } else {
        items.push_back({counting ? std::format("Starting in {}", v.countdown)
                                  : std::string(v.we_ready ? "Cancel Ready" : "Ready Up"),
                         v.connected && v.choice != 0 && v.save_ready && !counting, v.we_ready,
                         [] { net::lobby_toggle_ready(); }});
        items.push_back({"Leave the Party", v.connected && !counting, false, [] { input::trigger(input::Action::Leave); }});
    }
    items.push_back({"Co-op Settings", true, false, [] { coop_page_toggle(); }});
    items.push_back({"Play Solo", !counting, false, [] { net::lobby_dismiss(); }});
    return items;
}

// Draws the lobby and takes its input; false when it is not on screen.
bool draw_lobby(float width, float height, ImDrawList* dl, ImVec2 mouse, const std::vector<ImVec2>& clicks,
                const std::vector<unsigned>& keys) {
    const net::LobbyView v = net::lobby_view();
    g_lobby_shown = v.show && !g_open.load();
    if (!v.show || g_open.load()) return false;
    const float s = height / 1080.0f;
    auto items = lobby_items(v);
    const int n = static_cast<int>(items.size());
    if (g_lobby_sel >= n) g_lobby_sel = 0;
    // input
    static uint16_t pad_before = 0;
    const uint16_t pad = input::pad_buttons();
    const uint16_t pressed = pad & ~pad_before;
    pad_before = pad;
    input::mask_from_game(input::kPadDpad | input::kPadA | input::kPadB | input::kPadStart, 150);
    auto move = [&](int d) {
        for (int i = 0; i < n; ++i) {
            g_lobby_sel = (g_lobby_sel + d + n) % n;
            if (items[g_lobby_sel].enabled) break;
        }
    };
    auto act = [&](int i) {
        if (i >= 0 && i < n && items[i].enabled && items[i].act) {
            log::info("Co-op lobby: {}", items[i].label);
            items[i].act();
        }
    };
    for (unsigned k : keys) {
        if (k == VK_UP) move(-1);
        else if (k == VK_DOWN) move(1);
        else if (k == VK_RETURN || k == VK_SPACE) act(g_lobby_sel);
        else if (k == VK_ESCAPE || k == VK_BACK) net::lobby_dismiss();
    }
    if (pressed & input::kPadUp) move(-1);
    if (pressed & input::kPadDown) move(1);
    if (pressed & input::kPadA) act(g_lobby_sel);
    if (pressed & input::kPadB) net::lobby_dismiss();
    const float x0 = 130 * s, y0 = 360 * s, item_h = 64 * s, item_size = 34 * s;
    for (int i = 0; i < n; ++i) {
        const float y = y0 + i * item_h;
        const float w = text_w(item_size, items[i].label) + 60 * s;
        const bool over = inside(mouse, x0 - 30 * s, y - 8 * s, x0 + w, y + item_h - 14 * s);
        if (over && items[i].enabled && input::method() == input::Method::Keyboard) g_lobby_sel = i;
        for (const ImVec2& c : clicks) {
            if (inside(c, x0 - 30 * s, y - 8 * s, x0 + w, y + item_h - 14 * s)) act(i);
        }
    }
    // drawing: a dark side over the title's left, the menu, the party, the countdown
    dl->AddRectFilledMultiColor(ImVec2(0, 0), ImVec2(width * 0.5f, height), IM_COL32(6, 5, 4, 236), IM_COL32(6, 5, 4, 0),
                                IM_COL32(6, 5, 4, 0), IM_COL32(6, 5, 4, 236));
    text(dl, x0, 200 * s, 54 * s, "Seamless Co-op", kGold, true);
    text(dl, x0 + 2 * s, 268 * s, 24 * s,
         v.host ? "Crimson Desert, together. You host." : "Crimson Desert, together. You join your friend's world.", kDim);
    dl->AddLine(ImVec2(x0, 318 * s), ImVec2(x0 + 520 * s, 318 * s), kRule, 1.0f);
    for (int i = 0; i < n; ++i) {
        const LobbyItem& it = items[i];
        const float y = y0 + i * item_h;
        const bool sel = i == g_lobby_sel && it.enabled;
        const ImU32 c = !it.enabled ? IM_COL32(150, 144, 134, 120) : sel ? kGold : kText;
        if (sel) {
            dl->AddRectFilledMultiColor(ImVec2(x0 - 30 * s, y - 6 * s), ImVec2(x0 + 560 * s, y + item_h - 16 * s),
                                        IM_COL32(232, 200, 130, 40), IM_COL32(232, 200, 130, 0),
                                        IM_COL32(232, 200, 130, 0), IM_COL32(232, 200, 130, 40));
            const float cy = y + item_h * 0.32f;
            dl->AddQuadFilled(ImVec2(x0 - 16 * s, cy - 6 * s), ImVec2(x0 - 10 * s, cy), ImVec2(x0 - 16 * s, cy + 6 * s),
                              ImVec2(x0 - 22 * s, cy), kGold);
        }
        const float tw = text(dl, x0, y, item_size, it.label, c);
        if (it.chosen) text(dl, x0 + tw + 18 * s, y + 6 * s, 22 * s, v.host ? "chosen" : "ready", kGold);
    }
    wrap(dl, x0, y0 + n * item_h + 24 * s, 560 * s, 23 * s, v.status, kDim);
    if (!v.worlds_on) {
        wrap(dl, x0, y0 + n * item_h + 90 * s, 560 * s, 20 * s,
             "Co-op saves are off: you play in your own saves. Turn them on in Co-op Settings > Saves & Story for a "
             "separate save with each friend.", IM_COL32(150, 144, 134, 255));
    }
    // the party, top right (under the game's version)
    {
        const float right = width - 70 * s, top = 92 * s, size = 26 * s;
        const std::string head = "Party";
        text(dl, right - text_w(size, head, true), top, size, head, kGold, true);
        dl->AddLine(ImVec2(right - 330 * s, top + 40 * s), ImVec2(right, top + 40 * s), kRule, 1.0f);
        struct Member {
            std::string name, tag;
            ImU32 colour;
        };
        std::vector<Member> members;
        members.push_back({v.own_name, v.host ? "Host" : (v.we_ready ? "Guest, ready" : "Guest"), kText});
        if (v.connected) {
            members.push_back({v.partner_name, v.host ? (v.partner_ready ? "Guest, ready" : "Guest, not ready") : "Host",
                               ui::partner_colour()});
        }
        float y = top + 56 * s;
        for (const Member& m : members) {
            const float tw = text_w(size, m.name);
            text(dl, right - tw, y, size, m.name, m.colour);
            const float cx = right - tw - 18 * s, cy = y + size * 0.55f;
            dl->AddQuadFilled(ImVec2(cx, cy - 6 * s), ImVec2(cx + 6 * s, cy), ImVec2(cx, cy + 6 * s), ImVec2(cx - 6 * s, cy),
                              m.colour);
            text(dl, right - text_w(19 * s, m.tag), y + size + 6 * s, 19 * s, m.tag, kDim);
            y += size + 42 * s;
        }
        if (!v.connected) {
            const std::string w = v.host ? "Waiting for a friend" : "Looking for the host";
            text(dl, right - text_w(21 * s, w), y, 21 * s, w, kDim);
        }
    }
    // the title's own prompts are not ours while the lobby is up: covered, ours instead
    dl->AddRectFilledMultiColor(ImVec2(width * 0.45f, height - 110 * s), ImVec2(width, height), IM_COL32(6, 5, 4, 0),
                                IM_COL32(6, 5, 4, 250), IM_COL32(6, 5, 4, 250), IM_COL32(6, 5, 4, 0));
    {
        float right = width - 60 * s;
        const float y = height - 64 * s, size = 20 * s;
        if (input::method() == input::Method::Keyboard) {
            right = hint(dl, right, y, size, "Esc", "Play Solo");
            hint(dl, right, y, size, "Enter", "Select");
        } else {
            right = hint(dl, right, y, size, "B", "Play Solo");
            hint(dl, right, y, size, "A", "Select");
        }
    }
    if (v.countdown >= 0) {
        ui::title_card(width, height, std::to_string(std::max(1, v.countdown)), "Both games start together", kGold);
    }
    return true;
}

}  // namespace

bool coop_page_open() { return g_open.load(); }
bool coop_page_details() { return g_details.load(); }

void coop_page_toggle() {
    const bool now = !g_open.load();
    g_open = now;
    if (now) {
        g_focus_rows = false;
        std::lock_guard lock(g_in_mutex);
        g_keys.clear();
        g_clicks.clear();
        g_wheel = 0;
    }
    log::info("Co-op page {}", now ? "opened" : "closed");
}

bool coop_page_hook_key(unsigned vk, bool down) {
    if (!game_in_front()) return false;
    if (!g_open.load() && g_lobby_shown.load()) {
        // The title lobby: its keys, and the title's own (Play, New Game, Load, Settings, Exit) kept from the game.
        switch (vk) {
            case VK_UP: case VK_DOWN: case VK_LEFT: case VK_RIGHT: case VK_RETURN: case VK_SPACE: case VK_ESCAPE:
            case VK_BACK: case 'E': case 'Z': case 'F': case 'O':
                if (down) {
                    std::lock_guard lock(g_in_mutex);
                    if (g_keys.size() < 32) g_keys.push_back(vk);
                }
                return true;
            default:
                return false;
        }
    }
    if (!g_open.load()) return false;
    switch (vk) {
        case VK_UP: case VK_DOWN: case VK_LEFT: case VK_RIGHT: case VK_TAB: case VK_RETURN: case VK_SPACE:
        case VK_ESCAPE: case VK_BACK: case 'Q': case 'E': case VK_PRIOR: case VK_NEXT:
            if (down) {
                std::lock_guard lock(g_in_mutex);
                if (g_keys.size() < 32) g_keys.push_back(vk);
            }
            return true;
        default:
            return false;
    }
}

void coop_page_draw(float width, float height, const HudData& d, const std::vector<std::string>& status) {
    start_mouse_hook();
    const HWND fg = GetForegroundWindow();
    if (game_in_front()) g_hwnd = fg;
    const Space sp{width, height, height / 1080.0f};
    ImDrawList* dl = ImGui::GetForegroundDrawList();
    // The mouse, in the overlay's (client) coordinates.
    ImVec2 mouse(-1, -1);
    {
        POINT p{};
        if (GetCursorPos(&p) && g_hwnd.load() && ScreenToClient(g_hwnd.load(), &p)) mouse = ImVec2(float(p.x), float(p.y));
    }
    std::vector<ImVec2> clicks;
    int wheel = 0;
    std::vector<unsigned> keys;
    {
        std::lock_guard lock(g_in_mutex);
        for (POINT p : g_clicks) {
            if (g_hwnd.load() && ScreenToClient(g_hwnd.load(), &p)) clicks.push_back(ImVec2(float(p.x), float(p.y)));
        }
        g_clicks.clear();
        wheel = g_wheel;
        g_wheel = 0;
        keys.assign(g_keys.begin(), g_keys.end());
        g_keys.clear();
    }

    // ---- the title lobby (before anyone plays)
    if (!g_open.load() && draw_lobby(width, height, dl, mouse, clicks, keys)) return;
    // ---- a different difficulty from the friend's (net/coop_lobby.h)
    if (!g_open.load()) {
        const std::string card = net::lobby_difficulty_card();
        if (!card.empty()) {
            const auto nl = card.find('\n');
            ui::band(width, height * 0.16f, card.substr(0, nl), nl == std::string::npos ? card : card.substr(nl + 1));
        }
    }

    // ---- the tab after the game's "Others", while its menu is open
    const bool menu = game::ui_menu_open();
    const float tab_size = 31.0f * sp.s;
    const std::string tab = "Co-op";
    const float tab_w = text_w(tab_size, tab);
    const float tab_x = sp.x(964) - tab_w * 0.5f, tab_y = sp.y(50) - tab_size * 0.55f;
    if (menu || g_open.load()) {
        const bool hover = inside(mouse, tab_x - 20 * sp.s, tab_y - 10 * sp.s, tab_x + tab_w + 20 * sp.s, tab_y + 60 * sp.s);
        if (!g_open.load()) {
            text(dl, tab_x, tab_y, tab_size, tab, hover ? kGold : kText);
            const std::string key = "{Panel}";
            const float kw = text_w(17.0f * sp.s, key);
            ui::rich_text(dl, sp.x(964) - kw * 0.5f, sp.y(74), 17.0f * sp.s, key, kDim);
            // On screen (for the mouse hook): the label and its key.
            POINT a{static_cast<LONG>(tab_x - 20 * sp.s), static_cast<LONG>(tab_y - 10 * sp.s)};
            POINT b{static_cast<LONG>(tab_x + tab_w + 20 * sp.s), static_cast<LONG>(sp.y(96))};
            if (g_hwnd.load()) {
                ClientToScreen(g_hwnd.load(), &a);
                ClientToScreen(g_hwnd.load(), &b);
            }
            g_tab_l = a.x, g_tab_t = a.y, g_tab_r = b.x, g_tab_b = b.y;
            g_tab_shown = menu;
            for (const ImVec2& c : clicks) {
                if (inside(c, tab_x - 20 * sp.s, tab_y - 10 * sp.s, tab_x + tab_w + 20 * sp.s, sp.y(96))) coop_page_toggle();
            }
        }
    } else {
        g_tab_shown = false;
    }
    if (!g_open.load()) return;
    g_tab_shown = false;

    // ---- input: keys, pad, mouse
    static uint16_t pad_before = 0;
    const uint16_t pad = input::pad_buttons();
    const uint16_t pressed = pad & ~pad_before;
    pad_before = pad;
    input::mask_from_game(input::kPadDpad | input::kPadA | input::kPadB | input::kPadLB | input::kPadRB, 150);
    std::vector<Row> rows = rows_for(g_section, d, status);
    const int n = static_cast<int>(rows.size());
    auto first_selectable = [&] {
        for (int i = 0; i < n; ++i) {
            if (rows[i].kind != Row::Info) return i;
        }
        return 0;
    };
    auto change_section = [&](int delta) {
        g_section = (g_section + delta + kSectionCount) % kSectionCount;
        rows = rows_for(g_section, d, status);
        g_row = 0;
        g_top = 0;
    };
    auto move_row = [&](int delta) {
        const int m = static_cast<int>(rows.size());
        if (!m) return;
        for (int step = 0; step < m; ++step) {
            g_row = (g_row + delta + m) % m;
            if (rows[g_row].kind != Row::Info || g_section == 7 || g_section == 8) break;
        }
    };
    auto activate = [&](int delta) {
        if (g_row < 0 || g_row >= static_cast<int>(rows.size())) return;
        Row& r = rows[g_row];
        if (r.act && (r.kind == Row::Button ? delta == 0 : true)) r.act(delta);
        if (r.kind == Row::Button && delta == 0) log::info("Co-op page: {}", r.label);
    };
    bool close = false;
    for (unsigned k : keys) {
        switch (k) {
            case VK_ESCAPE: case VK_BACK:
                if (g_focus_rows) g_focus_rows = false;
                else close = true;
                break;
            case VK_TAB: case 'E': case VK_NEXT: change_section(1); break;
            case 'Q': case VK_PRIOR: change_section(-1); break;
            case VK_UP:
                if (g_focus_rows) move_row(-1);
                else change_section(-1);
                break;
            case VK_DOWN:
                if (g_focus_rows) move_row(1);
                else change_section(1);
                break;
            case VK_RIGHT:
                if (!g_focus_rows) {
                    g_focus_rows = true;
                    g_row = first_selectable();
                } else if (rows[g_row].kind == Row::Setting) {
                    activate(1);
                }
                break;
            case VK_LEFT:
                if (g_focus_rows && rows[g_row].kind == Row::Setting) activate(-1);
                else g_focus_rows = false;
                break;
            case VK_RETURN: case VK_SPACE:
                if (!g_focus_rows) {
                    g_focus_rows = true;
                    g_row = first_selectable();
                } else {
                    activate(0);
                }
                break;
        }
    }
    if (pressed & input::kPadB) {
        if (g_focus_rows) g_focus_rows = false;
        else close = true;
    }
    if (pressed & input::kPadRB) change_section(1);
    if (pressed & input::kPadLB) change_section(-1);
    if (pressed & input::kPadUp) g_focus_rows ? move_row(-1) : change_section(-1);
    if (pressed & input::kPadDown) g_focus_rows ? move_row(1) : change_section(1);
    if (pressed & input::kPadRight) {
        if (!g_focus_rows) {
            g_focus_rows = true;
            g_row = first_selectable();
        } else if (!rows.empty() && rows[g_row].kind == Row::Setting) {
            activate(1);
        }
    }
    if (pressed & input::kPadLeft) {
        if (g_focus_rows && !rows.empty() && rows[g_row].kind == Row::Setting) activate(-1);
        else g_focus_rows = false;
    }
    if (pressed & input::kPadA) {
        if (!g_focus_rows) {
            g_focus_rows = true;
            g_row = first_selectable();
        } else {
            activate(0);
        }
    }

    // ---- layout
    const float L = sp.x(230), R = sp.x(1690), T = sp.y(150), B = sp.y(910);
    const float sec_x = sp.x(270), sec_y = sp.y(285), sec_h = 54 * sp.s, sec_size = 27 * sp.s;
    const float div_x = sp.x(610);
    const float row_l = sp.x(640), row_r = sp.x(1650), row_y = sp.y(285), row_h = 52 * sp.s, row_size = 25 * sp.s;
    const int visible = std::max(1, static_cast<int>((sp.y(790) - row_y) / row_h));

    // mouse: hover picks, click acts, wheel scrolls the rows
    const bool use_mouse = input::method() == input::Method::Keyboard;
    for (int i = 0; i < kSectionCount; ++i) {
        const float y = sec_y + i * sec_h;
        const bool over = inside(mouse, sec_x - 10 * sp.s, y - 6 * sp.s, div_x - 10 * sp.s, y + sec_h - 6 * sp.s);
        for (const ImVec2& c : clicks) {
            if (inside(c, sec_x - 10 * sp.s, y - 6 * sp.s, div_x - 10 * sp.s, y + sec_h - 6 * sp.s) && i != g_section) {
                change_section(i - g_section);
                g_focus_rows = false;
            }
        }
        (void)over;
    }
    rows = rows_for(g_section, d, status);
    const int rn = static_cast<int>(rows.size());
    if (g_row >= rn) g_row = std::max(0, rn - 1);
    if (wheel) g_top = std::clamp(g_top - wheel, 0, std::max(0, rn - visible));
    if (g_focus_rows) {
        if (g_row < g_top) g_top = g_row;
        if (g_row >= g_top + visible) g_top = g_row - visible + 1;
    }
    for (int i = g_top; i < std::min(rn, g_top + visible); ++i) {
        const float y = row_y + (i - g_top) * row_h;
        const bool over = inside(mouse, row_l - 10 * sp.s, y - 8 * sp.s, row_r + 10 * sp.s, y + row_h - 8 * sp.s);
        if (over && use_mouse && rows[i].kind != Row::Info) {
            g_row = i;
            g_focus_rows = true;
        }
        for (const ImVec2& c : clicks) {
            if (!inside(c, row_l - 10 * sp.s, y - 8 * sp.s, row_r + 10 * sp.s, y + row_h - 8 * sp.s)) continue;
            g_row = i;
            g_focus_rows = true;
            if (rows[i].kind == Row::Button) {
                activate(0);
            } else if (rows[i].kind == Row::Setting) {
                // on the value: its "<" goes back, the rest forward; on the label: forward
                const std::string v = "<   " + rows[i].value + "   >";
                const float vl = row_r - text_w(row_size, v);
                activate(c.x >= vl && c.x < vl + text_w(row_size, "<   ") ? -1 : 1);
            }
            rows = rows_for(g_section, d, status);
        }
    }

    // ---- drawing
    dl->AddRectFilled(ImVec2(0, 0), ImVec2(width, height), IM_COL32(8, 7, 6, 205));
    // our tab, selected (gold, a dot above, a light rule under it, like "Others" when it is chosen)
    text(dl, tab_x, tab_y, tab_size, tab, kGold);
    dl->AddCircleFilled(ImVec2(sp.x(964), sp.y(22)), 3.5f * sp.s, IM_COL32(236, 160, 70, 255));
    dl->AddLine(ImVec2(tab_x - 40 * sp.s, sp.y(78)), ImVec2(tab_x + tab_w + 40 * sp.s, sp.y(78)), kGold, 1.5f * sp.s);
    // the frame
    dl->AddRectFilled(ImVec2(L, T), ImVec2(R, B), IM_COL32(22, 20, 18, 238));
    dl->AddRect(ImVec2(L, T), ImVec2(R, B), kRule, 0.0f, 1.0f);
    text(dl, sp.x(270), sp.y(180), 40 * sp.s, "Co-op", kGold, true);
    {
        const std::string who = d.connected ? "Playing with " + d.peer_name + "   " + connection_text(d) : "Not in a session";
        const float w = text_w(23 * sp.s, who);
        text(dl, sp.x(1650) - w, sp.y(196), 23 * sp.s, who, d.connected ? kText : kDim);
    }
    dl->AddLine(ImVec2(sp.x(270), sp.y(250)), ImVec2(sp.x(1650), sp.y(250)), kRule, 1.0f);
    // sections
    for (int i = 0; i < kSectionCount; ++i) {
        const float y = sec_y + i * sec_h;
        const bool sel = i == g_section;
        const bool over = inside(mouse, sec_x - 10 * sp.s, y - 6 * sp.s, div_x - 10 * sp.s, y + sec_h - 6 * sp.s);
        if (sel) {
            dl->AddRectFilled(ImVec2(sec_x - 14 * sp.s, y - 6 * sp.s), ImVec2(div_x - 14 * sp.s, y + sec_h - 10 * sp.s),
                              IM_COL32(232, 200, 130, g_focus_rows ? 14 : 30));
            dl->AddRectFilled(ImVec2(sec_x - 14 * sp.s, y - 6 * sp.s), ImVec2(sec_x - 11 * sp.s, y + sec_h - 10 * sp.s),
                              kGold);
        }
        text(dl, sec_x, y + 4 * sp.s, sec_size, kSections[i], sel ? kGold : over ? kText : kDim);
    }
    dl->AddLine(ImVec2(div_x, sp.y(280)), ImVec2(div_x, sp.y(800)), kRule, 1.0f);
    // rows
    for (int i = g_top; i < std::min(rn, g_top + visible); ++i) {
        const Row& r = rows[i];
        const float y = row_y + (i - g_top) * row_h;
        const bool sel = g_focus_rows && i == g_row && r.kind != Row::Info;
        if (sel) {
            dl->AddRectFilled(ImVec2(row_l - 10 * sp.s, y - 6 * sp.s), ImVec2(row_r + 10 * sp.s, y + row_h - 10 * sp.s),
                              IM_COL32(232, 200, 130, 26));
            dl->AddLine(ImVec2(row_l - 10 * sp.s, y - 6 * sp.s), ImVec2(row_r + 10 * sp.s, y - 6 * sp.s),
                        IM_COL32(232, 200, 130, 150), 1.0f);
            dl->AddLine(ImVec2(row_l - 10 * sp.s, y + row_h - 10 * sp.s), ImVec2(row_r + 10 * sp.s, y + row_h - 10 * sp.s),
                        IM_COL32(232, 200, 130, 150), 1.0f);
        }
        const ImU32 lc = r.kind == Row::Info && g_section != 7 ? kDim : sel ? kGold : kText;
        text(dl, row_l, y + 4 * sp.s, row_size, r.label, lc);
        if (!r.value.empty()) {
            std::string v = r.value;
            if (r.kind == Row::Setting) v = "<   " + v + "   >";
            const float vw = text_w(row_size, v);
            text(dl, row_r - vw, y + 4 * sp.s, row_size, v, r.kind == Row::Setting ? (sel ? kGold : kText) : kDim);
        }
    }
    if (rn > visible) {
        const std::string more = std::format("{} - {} of {}", g_top + 1, std::min(rn, g_top + visible), rn);
        text(dl, row_r - text_w(18 * sp.s, more), sp.y(790), 18 * sp.s, more, kDim);
    }
    // the chosen row's explanation
    dl->AddLine(ImVec2(sp.x(270), sp.y(815)), ImVec2(sp.x(1650), sp.y(815)), kRule, 1.0f);
    if (g_focus_rows && g_row < rn) {
        std::string help = rows[g_row].help;
        if (!rows[g_row].live) help += help.empty() ? "Applies at the next start." : " Applies at the next start.";
        if (!help.empty()) wrap(dl, sp.x(270), sp.y(830), sp.x(1650) - sp.x(270), 21 * sp.s, help, kDim);
    } else {
        wrap(dl, sp.x(270), sp.y(830), sp.x(1650) - sp.x(270), 21 * sp.s,
             "Everything Co-op+ does, in one place. Changes are saved at once.", kDim);
    }
    // key hints, bottom right (the game's own place for them), over a band that hides the game's own hints
    dl->AddRectFilledMultiColor(ImVec2(sp.x(900), sp.y(985)), ImVec2(width, height), IM_COL32(8, 7, 6, 0),
                                IM_COL32(8, 7, 6, 245), IM_COL32(8, 7, 6, 245), IM_COL32(8, 7, 6, 0));
    {
        float right = sp.x(1880);
        const float y = sp.y(1032), size = 19 * sp.s;
        if (input::method() == input::Method::Keyboard) {
            right = hint(dl, right, y, size, "Esc", "Close");
            right = hint(dl, right, y, size, "Tab", "Section");
            right = hint(dl, right, y, size, "Enter", "Select");
            hint(dl, right, y, size, "<  >", "Change");
        } else {
            right = hint(dl, right, y, size, "B", "Back");
            right = hint(dl, right, y, size, "LB RB", "Section");
            right = hint(dl, right, y, size, "A", "Select");
            hint(dl, right, y, size, "D-pad", "Change");
        }
    }
    if (close) coop_page_toggle();
}

}  // namespace cdmp::render
