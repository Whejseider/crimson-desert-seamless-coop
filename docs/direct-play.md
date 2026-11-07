# Direct play (Radmin VPN, LAN, any VPN) — no Steam invite needed

Co-op+ can connect two games straight to each other over a virtual LAN such as **Radmin VPN** (also Hamachi,
ZeroTier, Tailscale) or a real LAN. Requested by Plitos (2026-11-07); built into every release since then.

## Setup (both players)
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
