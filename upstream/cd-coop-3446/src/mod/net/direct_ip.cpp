#include "mod/net/direct_ip.h"

#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <windows.h>

#include <bcrypt.h>
#include <intrin.h>

#include <cstdio>
#include <cstring>
#include <mutex>
#include <utility>
#include <vector>

#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "bcrypt.lib")

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

std::optional<uint32_t> direct_host_address(std::string& name, std::string& why) {
    const auto all = local_ipv4();
    for (const auto& l : all) {
        if (radmin(l.ip)) {
            name = narrow(l.name) + " (Radmin VPN)";
            return l.ip;
        }
    }
    // Another VPN or a LAN: a private / CGNAT address (Hamachi uses 25.x).
    for (const auto& l : all) {
        const uint32_t h = ntohl(l.ip);
        const bool priv = (h >> 24) == 10 || (h >> 20) == 0xAC1 || (h >> 16) == 0xC0A8 || (h >> 22) == 0x191 ||
                          (h >> 24) == 25;
        if (priv) {
            name = narrow(l.name);
            return l.ip;
        }
    }
    why = "no Radmin VPN (or other VPN / LAN) address on this PC: start Radmin VPN and join your network";
    return std::nullopt;
}

std::string direct_dotted(uint32_t ip) { return dotted(ip); }

uint64_t direct_new_token() {
    uint64_t t = 0;
    while (!t) {
        if (BCryptGenRandom(nullptr, reinterpret_cast<PUCHAR>(&t), sizeof t, BCRYPT_USE_SYSTEM_PREFERRED_RNG) != 0) {
            t = (static_cast<uint64_t>(GetTickCount64()) << 32) ^ __rdtsc();  // not expected: the system RNG failed
        }
    }
    return t;
}

namespace {
std::mutex g_req_mutex;
std::optional<DirectRequest> g_request;
DirectState g_state;
}  // namespace

void direct_request(const DirectRequest& r) {
    std::lock_guard lock(g_req_mutex);
    g_request = r;
}

std::optional<DirectRequest> direct_take_request() {
    std::lock_guard lock(g_req_mutex);
    auto r = g_request;
    g_request.reset();
    return r;
}

void direct_set_state(const DirectState& s) {
    std::lock_guard lock(g_req_mutex);
    g_state = s;
}

DirectState direct_state() {
    std::lock_guard lock(g_req_mutex);
    return g_state;
}

bool direct_clipboard_set(const std::string& text) {
    if (!OpenClipboard(nullptr)) return false;
    EmptyClipboard();
    const std::wstring w(text.begin(), text.end());  // ASCII only
    HGLOBAL h = GlobalAlloc(GMEM_MOVEABLE, (w.size() + 1) * sizeof(wchar_t));
    bool ok = false;
    if (h) {
        std::memcpy(GlobalLock(h), w.c_str(), (w.size() + 1) * sizeof(wchar_t));
        GlobalUnlock(h);
        ok = SetClipboardData(CF_UNICODETEXT, h) != nullptr;
        if (!ok) GlobalFree(h);
    }
    CloseClipboard();
    return ok;
}

std::string direct_clipboard_get() {
    std::string out;
    if (!OpenClipboard(nullptr)) return out;
    if (HANDLE h = GetClipboardData(CF_UNICODETEXT)) {
        if (const auto* w = static_cast<const wchar_t*>(GlobalLock(h))) {
            for (size_t i = 0; w[i] && i < 4096; ++i) out += w[i] < 128 ? static_cast<char>(w[i]) : ' ';
            GlobalUnlock(h);
        }
    }
    CloseClipboard();
    return out;
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
