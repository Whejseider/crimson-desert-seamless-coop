#pragma once
// Co-op+ direct play (Plitos 2026-11-07: "a way to connect with his friend via Radmin VPN", in every release): the two
// games talk UDP straight to each other over a VPN (Radmin VPN, Hamachi, ZeroTier, Tailscale) or a LAN, no Steam
// needed. [net] transport=direct (radmin and lan are the same), role=host|guest, peer_host = the FRIEND's address
// (on both sides), peer_port (the host's, default 37015), bind_host (optional: our own address to use).
//
// Safety: the socket is bound only to the adapter that reaches the friend (never to every interface), and anything
// not sent from the friend's address is dropped unread (transport.h, UdpTransport::open_direct).

#include <cstdint>
#include <optional>
#include <string>

#include "mod/net/direct_invite.h"

namespace cdmp::net {

// Our own IPv4 address (network byte order) on the adapter whose subnet holds `peer_ip`; else our Radmin VPN address
// (26.x.x.x); nullopt if none. `why` says which.
std::optional<uint32_t> direct_local_address_for(uint32_t peer_ip, std::string& why);
// Our addresses worth telling the friend (VPN and LAN adapters that are up), for the log and the panel.
std::string direct_own_addresses();

// ---- The co-op menu's "Radmin VPN / Direct IP" (2026-11-07, Plitos's handoff): Host and Join without editing the ini.
// Steam stays the default; the choice lasts until Back to Steam, Leave, or the game closes (the ini is not changed).

// Our address to host on: the Radmin VPN one (26.x) if there is one, else another VPN / LAN adapter's (Hamachi,
// ZeroTier, Tailscale, a LAN); nullopt with `why` if none. `name` = the adapter's name.
std::optional<uint32_t> direct_host_address(std::string& name, std::string& why);
std::string direct_dotted(uint32_t ip);

// The invite (its format and parser): direct_invite.h.
uint64_t direct_new_token();

// The menu (render thread) asks; the network loop takes it and restarts the session on the new transport.
struct DirectRequest {
    enum class Kind : uint8_t { Host, Join, Steam } kind = Kind::Steam;
    DirectTarget target;    // Host: our own address / port / token; Join: the host's
    uint32_t peer_ip = 0;   // Host only: a guest address also let in without a knock (0 = invite only)
};
void direct_request(const DirectRequest& r);
std::optional<DirectRequest> direct_take_request();

// What the menu shows (network loop writes, render thread reads).
struct DirectState {
    bool active = false;    // this session is a direct one
    bool host = false;
    std::string invite;     // host: the invite to send ("" for an ini-configured session)
    std::string address;    // our address on that network
    std::string friend_address;
    std::string error;      // why the last Host / Join could not start ("" = fine)
};
void direct_set_state(const DirectState& s);
DirectState direct_state();

// The Windows clipboard (render thread).
bool direct_clipboard_set(const std::string& text);
std::string direct_clipboard_get();

}  // namespace cdmp::net
