#include "net/steam_runtime.hpp"

#include <windows.h>

namespace cdcoop::net {
namespace {
template <class F>
F resolve(HMODULE m, const char* name, std::string& missing) {
    auto p = reinterpret_cast<F>(reinterpret_cast<void*>(GetProcAddress(m, name)));
    if (!p) missing += std::string(name) + " ";
    return p;
}
// Accessor versions differ between SDKs; try the newest first.
void* first_accessor(HMODULE m, std::initializer_list<const char*> names) {
    for (const char* n : names)
        if (auto f = reinterpret_cast<void* (*)()>(reinterpret_cast<void*>(GetProcAddress(m, n)))) return f();
    return nullptr;
}
}  // namespace

SteamStatus query_steam() {
    SteamStatus s;
    HMODULE m = GetModuleHandleW(L"steam_api64.dll");
    if (!m) return s;
    s.dll_loaded = true;
    auto is_running = resolve<bool (*)()>(m, "SteamAPI_IsSteamRunning", s.missing);
    auto get_user = resolve<std::int32_t (*)()>(m, "SteamAPI_GetHSteamUser", s.missing);
    s.running = is_running && is_running();
    if (!s.running || !get_user || get_user() == 0) return s;

    void* user = first_accessor(m, {"SteamAPI_SteamUser_v023", "SteamAPI_SteamUser_v022", "SteamAPI_SteamUser_v021"});
    void* friends = first_accessor(m, {"SteamAPI_SteamFriends_v018", "SteamAPI_SteamFriends_v017"});
    auto get_id = resolve<std::uint64_t (*)(void*)>(m, "SteamAPI_ISteamUser_GetSteamID", s.missing);
    auto get_name = resolve<const char* (*)(void*)>(m, "SteamAPI_ISteamFriends_GetPersonaName", s.missing);
    if (user && get_id) s.steam_id = get_id(user);
    if (friends && get_name) if (const char* n = get_name(friends)) s.persona_name = n;
    s.user_ready = s.steam_id != 0;

    s.matchmaking_api = GetProcAddress(m, "SteamAPI_SteamMatchmaking_v009") && GetProcAddress(m, "SteamAPI_ISteamMatchmaking_CreateLobby");
    s.messages_api = GetProcAddress(m, "SteamAPI_SteamNetworkingMessages_SteamAPI_v002") &&
                     GetProcAddress(m, "SteamAPI_ISteamNetworkingMessages_SendMessageToUser");
    if (!s.matchmaking_api) s.missing += "ISteamMatchmaking ";
    if (!s.messages_api) s.missing += "ISteamNetworkingMessages ";
    return s;
}
}  // namespace cdcoop::net
