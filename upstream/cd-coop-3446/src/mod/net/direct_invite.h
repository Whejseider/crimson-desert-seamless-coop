#pragma once
// Co-op+ (2026-11-07, Plitos's handoff "Copy invite / Paste invite"): the Radmin VPN / Direct IP invite, header-only
// and Windows-free so the unit tests cover it (tests/tests.cpp test_direct_invite).
//
// "coopplus1/<host address>:<port>/<16 hex digits>": the host's VPN endpoint and a 64-bit random secret, new for every
// Host press. Only someone holding the invite gets in (common/transport.h: the guest's first datagram is a knock with
// the secret); the address alone authenticates nobody. Still a direct connection: both players must be in the same
// VPN network. Addresses are IPv4 in network byte order (the first number in the lowest byte), as in sockaddr_in.

#include <cstdint>
#include <optional>
#include <string>
#include <utility>

namespace cdmp::net {

struct DirectTarget {
    uint32_t ip = 0;        // network byte order
    uint16_t port = 37015;
    uint64_t token = 0;     // 0: a bare address (the host must have this guest's address too)
};

inline std::string direct_dotted_ip(uint32_t ip) {
    return std::to_string(ip & 0xFF) + "." + std::to_string(ip >> 8 & 0xFF) + "." + std::to_string(ip >> 16 & 0xFF) +
           "." + std::to_string(ip >> 24 & 0xFF);
}

inline std::string direct_invite_encode(uint32_t ip, uint16_t port, uint64_t token) {
    static const char kHex[] = "0123456789abcdef";
    std::string t(16, '0');
    for (int i = 0; i < 16; ++i) t[15 - i] = kHex[(token >> (4 * i)) & 0xF];
    return "coopplus1/" + direct_dotted_ip(ip) + ":" + std::to_string(port) + "/" + t;
}

namespace direct_detail {
inline bool digit(char c) { return c >= '0' && c <= '9'; }
// "a.b.c.d" at `at` (each 0..255, not part of a longer number); the address (network byte order) and its length.
inline std::optional<std::pair<uint32_t, size_t>> ipv4_at(const std::string& s, size_t at) {
    if (at && (digit(s[at - 1]) || s[at - 1] == '.')) return std::nullopt;
    uint32_t parts[4] = {};
    size_t i = at;
    for (int p = 0; p < 4; ++p) {
        if (p && (i >= s.size() || s[i++] != '.')) return std::nullopt;
        size_t digits = 0;
        uint32_t v = 0;
        while (i < s.size() && digit(s[i]) && digits < 3) v = v * 10 + static_cast<uint32_t>(s[i++] - '0'), ++digits;
        if (!digits || v > 255) return std::nullopt;
        parts[p] = v;
    }
    if (i < s.size() && (digit(s[i]) || (s[i] == '.' && i + 1 < s.size() && digit(s[i + 1])))) return std::nullopt;
    return std::make_pair(parts[0] | parts[1] << 8 | parts[2] << 16 | parts[3] << 24, i - at);
}
// ":port" at `i` (moves `i` past it); 0 if there is none or it is out of range.
inline uint16_t port_at(const std::string& s, size_t& i) {
    if (i >= s.size() || s[i] != ':') return 0;
    size_t j = i + 1, d = 0;
    uint32_t v = 0;
    while (j < s.size() && digit(s[j]) && d < 5) v = v * 10 + static_cast<uint32_t>(s[j++] - '0'), ++d;
    if (!d || !v || v > 65535) return 0;
    i = j;
    return static_cast<uint16_t>(v);
}
}  // namespace direct_detail

// An invite, or a bare IPv4 address ("26.1.2.3" or "26.1.2.3:37015", Radmin VPN's "Copy IP address"), anywhere in
// `text` (pasted from a chat message). nullopt for anything else, a broken invite, 0.0.0.0 or 255.255.255.255.
inline std::optional<DirectTarget> direct_invite_parse(const std::string& text) {
    using namespace direct_detail;
    if (const size_t k = text.find("coopplus1/"); k != std::string::npos) {
        const size_t a = k + 10;
        const auto ip = ipv4_at(text, a);
        if (!ip || ip->first == 0 || ip->first == 0xFFFFFFFFu) return std::nullopt;
        size_t i = a + ip->second;
        DirectTarget t{ip->first, 37015, 0};
        if (const uint16_t p = port_at(text, i)) t.port = p;
        if (i >= text.size() || text[i] != '/') return std::nullopt;
        ++i;
        uint64_t token = 0;
        size_t d = 0;
        for (; i < text.size() && d < 16; ++i, ++d) {
            const char c = text[i];
            const int v = digit(c) ? c - '0' : c >= 'a' && c <= 'f' ? c - 'a' + 10 : c >= 'A' && c <= 'F' ? c - 'A' + 10 : -1;
            if (v < 0) break;
            token = token << 4 | static_cast<uint64_t>(v);
        }
        if (d != 16 || !token) return std::nullopt;
        if (i < text.size() && ((text[i] >= '0' && text[i] <= '9') || (text[i] >= 'a' && text[i] <= 'f') ||
                                (text[i] >= 'A' && text[i] <= 'F'))) {
            return std::nullopt;  // a 17th hex digit: not an invite of ours
        }
        t.token = token;
        return t;
    }
    for (size_t i = 0; i < text.size(); ++i) {
        if (!digit(text[i])) continue;
        const auto ip = ipv4_at(text, i);
        if (!ip) continue;
        if (ip->first == 0 || ip->first == 0xFFFFFFFFu) return std::nullopt;
        DirectTarget t{ip->first, 37015, 0};
        size_t j = i + ip->second;
        if (const uint16_t p = port_at(text, j)) t.port = p;
        return t;
    }
    return std::nullopt;
}

}  // namespace cdmp::net
