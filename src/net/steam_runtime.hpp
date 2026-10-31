#pragma once
// Steam access through the game's own steam_api64.dll and the Steamworks flat C API, resolved at runtime with
// GetProcAddress. We never call SteamAPI_Init ourselves (the game already did); we only attach to it. If any export
// is missing, co-op networking stays off and the game runs normally.
#include <cstdint>
#include <string>

namespace cdcoop::net {
struct SteamStatus {
    bool dll_loaded = false;
    bool running = false;
    bool user_ready = false;
    std::uint64_t steam_id = 0;
    std::string persona_name;
    bool matchmaking_api = false;  // lobbies
    bool messages_api = false;     // ISteamNetworkingMessages
    std::string missing;           // names of missing exports, for the log
};
// Safe to call repeatedly; returns user_ready=false until the game has initialised Steam.
SteamStatus query_steam();
}  // namespace cdcoop::net
