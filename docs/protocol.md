# Co-op+ wire protocol (v1)

Implementation: `src/net/protocol.{hpp,cpp}`, tests in `tests/test_protocol.cpp`.

Transport: Steam `ISteamNetworkingMessages` (P2P with Steam relay, no ports/IPs), channel 0 = reliable events,
channel 1 = unreliable snapshots. Steam lobby carries the session (join from overlay/friends list, invites, rich
presence `connect` string). Friends-only by default.

## Packet
| Field | Type | Notes |
|---|---|---|
| magic | 4 bytes | `CDC1` |
| version | u16 LE | `kProtocolVersion`; a mismatch is refused with a friendly "update the mod" message |
| type | u8 | `MsgType`; unknown types are skipped |
| length | u16 LE | payload bytes; packets stay under 1100 bytes (Steam unreliable MTU ~1200) |

Strings: u16 length + UTF-8 bytes, max 512. Floats are IEEE-754 LE; non-finite positions are rejected.

## Messages
| Type | Channel | Fields |
|---|---|---|
| 1 Hello | reliable | protocol_version, mod_version, game_build, steam_id, display_name, content_hash |
| 2 HelloReply | reliable | accepted, reason (ProtocolMismatch/GameVersionMismatch/SessionFull/ModMismatch), slot, message |
| 3 PlayerSnapshot | unreliable 10-20 Hz | tick, slot, pos[3], vel[3], yaw, anim_id, anim_time, move_flags, health_frac |
| 4 Chat | reliable | slot, text |
| 5 Ping | unreliable | nonce, send_time_us |
| 6 Leave | reliable | slot |

Planned (M2-M5): EntityOwnership, EnemyState, Hit, LootGrant, ProgressFlag, ConfigSync (host's gameplay TOML), Appearance.

## Rules
- Host-authoritative: the host owns enemies, loot rolls, scaling multipliers, progression merges, gameplay config.
- Never identify entities across PCs by pointer or by the game's per-load actor key; use mod-assigned ids mapped from
  stable spawn data.
- Validate every field from the network (sizes, counts, finite floats, rate limits) before it reaches game code.
- If the remote players' game builds differ, refuse with a clear message (both must be on the same patch).
