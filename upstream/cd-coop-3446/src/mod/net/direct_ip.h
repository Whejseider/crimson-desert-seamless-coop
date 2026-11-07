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

namespace cdmp::net {

// Our own IPv4 address (network byte order) on the adapter whose subnet holds `peer_ip`; else our Radmin VPN address
// (26.x.x.x); nullopt if none. `why` says which.
std::optional<uint32_t> direct_local_address_for(uint32_t peer_ip, std::string& why);
// Our addresses worth telling the friend (VPN and LAN adapters that are up), for the log and the panel.
std::string direct_own_addresses();

}  // namespace cdmp::net
