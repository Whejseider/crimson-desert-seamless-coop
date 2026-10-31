#pragma once
// Finds the lobby a Steam "Join Game" asks for, in a command line or a Steam
// connect string.
//
// Steam hands the lobby over in one of two ways, both as the text
// "+connect_lobby <lobby SteamID64>":
//   - the game is NOT running: Steam starts it with that text on the command
//     line (friends list "Join Game", or an accepted invite);
//   - the game IS running: steam_join.cpp receives GameLobbyJoinRequested_t (the
//     lobby id directly) or GameRichPresenceJoinRequested_t (our own rich
//     presence "connect" string, which uses the same text).
//
// Pure logic, no Windows and no Steam, so it is unit-tested (tests/tests.cpp).

#include <cstdint>
#include <optional>
#include <string_view>

namespace cdmp::net {

inline constexpr std::string_view kConnectLobbyArg = "+connect_lobby";

// A lobby CSteamID: universe Public (1) in bits 56-63, account type Chat (8) in
// bits 52-55. Anything else is not a lobby and is refused, so a stray number on
// the command line can never send the player into some other kind of id.
inline bool looks_like_lobby_id(uint64_t id) {
    return (id >> 56) == 1 && ((id >> 52) & 0xF) == 8 && (id & 0xFFFFFFFFull) != 0;
}

// The lobby id after "+connect_lobby" (separated by spaces, tabs or '='), or
// nothing. Works on narrow and wide text alike.
template <class Char>
std::optional<uint64_t> find_connect_lobby(std::basic_string_view<Char> text) {
    const size_t n = text.size();
    const size_t k = kConnectLobbyArg.size();
    for (size_t i = 0; i + k <= n; ++i) {
        bool match = true;
        for (size_t j = 0; j < k; ++j) {
            Char c = text[i + j];
            if (c >= 'A' && c <= 'Z') c = static_cast<Char>(c - 'A' + 'a');
            if (c != static_cast<Char>(kConnectLobbyArg[j])) {
                match = false;
                break;
            }
        }
        if (!match) continue;
        // The option must start a word: "x+connect_lobby" is not it.
        if (i > 0 && text[i - 1] != ' ' && text[i - 1] != '\t' && text[i - 1] != '"') continue;
        size_t p = i + k;
        if (p < n && text[p] != ' ' && text[p] != '\t' && text[p] != '=' && text[p] != '"') continue;
        while (p < n && (text[p] == ' ' || text[p] == '\t' || text[p] == '=' || text[p] == '"')) ++p;
        uint64_t id = 0;
        int digits = 0;
        bool overflow = false;
        while (p < n && text[p] >= '0' && text[p] <= '9') {
            const uint64_t d = static_cast<uint64_t>(text[p] - '0');
            if (id > (UINT64_MAX - d) / 10) overflow = true;
            id = id * 10 + d;
            ++digits;
            ++p;
        }
        // The number must end the word too: "123abc" is not a lobby id.
        if (p < n && text[p] != ' ' && text[p] != '\t' && text[p] != '"') continue;
        if (digits == 0 || digits > 20 || overflow || !looks_like_lobby_id(id)) continue;
        return id;
    }
    return std::nullopt;
}

inline std::optional<uint64_t> find_connect_lobby(std::string_view text) {
    return find_connect_lobby<char>(text);
}
inline std::optional<uint64_t> find_connect_lobby(std::wstring_view text) {
    return find_connect_lobby<wchar_t>(text);
}

}  // namespace cdmp::net
