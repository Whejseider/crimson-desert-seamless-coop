#include "mod/net/scoreboard.h"

#include <windows.h>

#include <cctype>
#include <format>
#include <mutex>

#include "mod/log.h"
#include "mod/net/coop_activity.h"

namespace cdmp::net {
namespace {

constexpr uint8_t kTitleType = 25;
struct GameName {
    const char* key;
    const char* plural;
};
constexpr GameName kGames[] = {{"duel", "duels"}, {"rps", "rock-paper-scissors"}, {"hunt", "hunts"},
                               {"race", "races"}, {"hide", "hide and seek"}, {"fish", "fishing contests"}, {"wrestle", "arm wrestling"}};

std::mutex g_mutex;
std::wstring g_path;
std::string g_partner_title, g_sent_title;
bool g_was_connected = false;

std::wstring wide(const std::string& s) { return std::wstring(s.begin(), s.end()); }

// A section name from a Steam name: letters, digits, space, _ - . only.
std::wstring section(const std::string& name) {
    std::wstring w;
    for (unsigned char c : name) w += (std::isalnum(c) || c == ' ' || c == '_' || c == '-' || c == '.') ? wchar_t(c) : L'_';
    return w.empty() ? L"_" : w;
}

int get(const std::wstring& sec, const std::string& key) {
    return static_cast<int>(GetPrivateProfileIntW(sec.c_str(), wide(key).c_str(), 0, g_path.c_str()));
}

void put(const std::wstring& sec, const std::string& key, int v) {
    WritePrivateProfileStringW(sec.c_str(), wide(key).c_str(), std::to_wstring(v).c_str(), g_path.c_str());
}

std::string title_locked() {
    if (g_path.empty()) return {};
    const int wins = get(L"_total", "wins"), duels = get(L"_total", "duel_wins"), races = get(L"_total", "race_wins"),
              hunts = get(L"_total", "hunt_wins");
    if (duels >= 10) return "the Duelist";
    if (races >= 10) return "the Swift";
    if (hunts >= 10) return "the Huntmaster";
    if (wins >= 25) return "the Champion";
    if (wins >= 3) return "the Sparring Partner";
    return {};
}

}  // namespace

void scoreboard_init(const std::wstring& path) {
    std::lock_guard lock(g_mutex);
    g_path = path;
}

void scoreboard_record(const std::string& friend_name, const std::string& game, bool won) {
    std::lock_guard lock(g_mutex);
    if (g_path.empty()) return;
    const std::wstring sec = section(friend_name);
    const std::string key = game + (won ? "_w" : "_l");
    put(sec, key, get(sec, key) + 1);
    if (won) {
        put(L"_total", "wins", get(L"_total", "wins") + 1);
        put(L"_total", game + "_wins", get(L"_total", game + "_wins") + 1);
    }
    log::info("Scoreboard: {} {} vs {} ({} {} - {})", won ? "won" : "lost", game, friend_name, game,
              get(sec, game + "_w"), get(sec, game + "_l"));
}

std::vector<std::string> scoreboard_lines(const std::string& friend_name) {
    std::vector<std::string> out;
    std::lock_guard lock(g_mutex);
    if (g_path.empty() || friend_name.empty()) return out;
    const std::wstring sec = section(friend_name);
    std::string line;
    for (const auto& g : kGames) {
        const int w = get(sec, std::string(g.key) + "_w"), l = get(sec, std::string(g.key) + "_l");
        if (w || l) line += std::format("{}{} {}-{}", line.empty() ? "" : ", ", g.plural, w, l);
    }
    out.push_back("Record vs " + friend_name + ": " + (line.empty() ? std::string("no challenges yet") : line));
    if (const std::string t = title_locked(); !t.empty()) out.push_back("Your title: " + t);
    return out;
}

std::string scoreboard_title() {
    std::lock_guard lock(g_mutex);
    return title_locked();
}

void scoreboard_on_event(const uint8_t* d, size_t n) {
    std::string t(reinterpret_cast<const char*>(d), std::min<size_t>(n, 40));
    for (char& c : t) {
        if (static_cast<unsigned char>(c) < 0x20 || static_cast<unsigned char>(c) > 0x7E) c = ' ';
    }
    std::lock_guard lock(g_mutex);
    g_partner_title = t;
}

std::string scoreboard_partner_title() {
    std::lock_guard lock(g_mutex);
    return g_partner_title;
}

void scoreboard_tick(bool connected) {
    std::string send;
    {
        std::lock_guard lock(g_mutex);
        if (!connected) {
            g_was_connected = false;
            g_partner_title.clear();
            return;
        }
        const std::string t = title_locked();
        if (!g_was_connected || t != g_sent_title) {
            g_was_connected = true;
            g_sent_title = t;
            send = t.empty() ? std::string(" ") : t;  // " " = no title (an empty event is still an event)
        }
    }
    if (!send.empty()) activity_send(kTitleType, std::vector<uint8_t>(send.begin(), send.end()));
}

}  // namespace cdmp::net
