# Direct play (Radmin VPN, LAN, any VPN) — no Steam invite needed

Co-op+ can connect two games straight to each other over a virtual LAN such as **Radmin VPN** (also Hamachi,
ZeroTier, Tailscale) or a real LAN. Requested by Plitos (2026-11-07); built into every release since then.

## The co-op menu (2026-11-07, from Plitos's handoff)

Steam invites stay the default. In the Co-op menu at the title, **Radmin VPN / Direct IP** opens a page with:

- **Host**: hosts on this PC's Radmin VPN address (26.x; else another VPN / LAN adapter), port 37015, and copies an
  invite to the clipboard: `coopplus1/<address>:37015/<16 hex digits>`. The hex part is a 64-bit random secret, new on
  every Host press. A bare address on the clipboard at that moment (the guest's, Radmin's Copy IP) is also let in.
- **Join**: reads the clipboard: an invite, or a bare address (`26.1.2.3`, `26.1.2.3:37015`), and joins.
- **Back to Steam Invite** (when a direct session is on), **Back**. Leave also returns to Steam.

While hosting, the main menu's Invite Friends becomes **Copy Invite**. The choice is not written to cdmp.ini: the next
game start is on Steam again.

How the invite is checked (`src/common/transport.h`, `UdpTransport::open_direct`): the guest's transport sends a 16-byte
knock (`CDKNOCK1` + the secret) before each datagram until the host has answered. The host lets in an address whose
knock carries the secret and drops everything else unread (wrong knocks are counted). Knocks never reach the game
protocol. The address in the invite authenticates nobody; the secret does, and it lasts only for that Host session.
Both players still need the same Co-op+ build (the Hello handshake checks it).

Tests: `tests/tests.cpp` test_direct_invite (format, parsing, bad invites, bare addresses) and test_direct_knock (wrong
secret dropped, right secret admitted, knocking stops once the host answered); two copies on one PC with
`[dev] direct_loopback=1` (the host's invite on 127.0.0.1).

## Setup in cdmp.ini (both players, without the menu)
1. Install Radmin VPN, and have both players join the same Radmin network. Each player's Radmin window shows their own
   address (26.x.x.x). Tell each other those addresses.
2. Open `cdmp.ini` in the game's `bin64` folder and set, under `[net]`:

   | Key | Host | Guest |
   | --- | --- | --- |
   | `transport` | `direct` | `direct` |
   | `role` | `host` | `guest` |
   | `peer_host` | the **guest's** Radmin address | the **host's** Radmin address |
   | `peer_port` | `37015` (the same on both) | `37015` |

   `radmin` and `lan` work as `transport` too (Plitos's patch used `transport=radmin`). `bind_host` is optional: your OWN
   address to use, when the mod picks the wrong adapter.
3. Start the game. The first time, Windows asks whether Crimson Desert may use the network: allow it (private
   networks). The host starts first; the guest connects as soon as both games are up.

## What it does and doesn't do
- The connection uses only the adapter that reaches your friend (the Radmin adapter, for example), never every network
  card, and anything not sent from your friend's address is dropped unread.
- Co-op saves work as with Steam: the co-op save with this friend is named after their VPN address
  (`Worlds\with-direct-26-…`), so keep the same Radmin network.
- Your name comes from Steam if Steam is running, else from `[general] player_name`.
- Steam invites and "Join Game" are Steam-only; with direct play the two games find each other through `peer_host`.
- Both players need the same Co-op+ version (as with Steam).
