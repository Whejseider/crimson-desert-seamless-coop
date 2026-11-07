#include "mod/net/direct_ip.h"

#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <windows.h>

#include <vector>

#pragma comment(lib, "iphlpapi.lib")

namespace cdmp::net {
namespace {

struct Local {
    uint32_t ip = 0;     // network order
    uint8_t prefix = 0;  // on-link prefix length
    std::wstring name;
};

std::vector<Local> local_ipv4() {
    std::vector<Local> out;
    ULONG size = 16 * 1024;
    std::vector<uint8_t> buf(size);
    auto* list = reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buf.data());
    ULONG flags = GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER;
    ULONG rc = GetAdaptersAddresses(AF_INET, flags, nullptr, list, &size);
    if (rc == ERROR_BUFFER_OVERFLOW) {
        buf.resize(size);
        list = reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buf.data());
        rc = GetAdaptersAddresses(AF_INET, flags, nullptr, list, &size);
    }
    if (rc != NO_ERROR) return out;
    for (auto* a = list; a; a = a->Next) {
        if (a->OperStatus != IfOperStatusUp || a->IfType == IF_TYPE_SOFTWARE_LOOPBACK) continue;
        for (auto* u = a->FirstUnicastAddress; u; u = u->Next) {
            if (!u->Address.lpSockaddr || u->Address.lpSockaddr->sa_family != AF_INET) continue;
            const auto* sin = reinterpret_cast<const sockaddr_in*>(u->Address.lpSockaddr);
            out.push_back({sin->sin_addr.s_addr, u->OnLinkPrefixLength, a->FriendlyName ? a->FriendlyName : L""});
        }
    }
    return out;
}

std::string dotted(uint32_t ip) {
    char b[INET_ADDRSTRLEN] = {};
    in_addr a{};
    a.s_addr = ip;
    inet_ntop(AF_INET, &a, b, sizeof b);
    return b;
}

std::string narrow(const std::wstring& w) {
    std::string s;
    for (wchar_t c : w) s += c < 128 ? static_cast<char>(c) : '?';
    return s;
}

bool radmin(uint32_t ip) { return (ntohl(ip) >> 24) == 26; }

}  // namespace

std::optional<uint32_t> direct_local_address_for(uint32_t peer_ip, std::string& why) {
    const auto all = local_ipv4();
    const uint32_t peer = ntohl(peer_ip);
    std::optional<Local> best;
    for (const auto& l : all) {
        if (l.prefix == 0 || l.prefix > 32) continue;
        const uint32_t mask = l.prefix == 32 ? 0xFFFFFFFFu : ~((1u << (32 - l.prefix)) - 1);
        if ((ntohl(l.ip) & mask) == (peer & mask) && (!best || l.prefix > best->prefix)) best = l;
    }
    if (best) {
        why = "the adapter '" + narrow(best->name) + "' reaches " + dotted(peer_ip);
        return best->ip;
    }
    for (const auto& l : all) {
        if (radmin(l.ip)) {
            why = "Radmin VPN adapter '" + narrow(l.name) + "'";
            return l.ip;
        }
    }
    why = "no adapter of this PC is on " + dotted(peer_ip) + "'s network (is the VPN connected?)";
    return std::nullopt;
}

std::string direct_own_addresses() {
    std::string s;
    for (const auto& l : local_ipv4()) {
        if (!s.empty()) s += ", ";
        s += dotted(l.ip) + (radmin(l.ip) ? " (Radmin VPN)" : "") + " on '" + narrow(l.name) + "'";
    }
    return s.empty() ? "none" : s;
}

}  // namespace cdmp::net
