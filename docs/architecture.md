# Architecture and milestone plan (draft 1, 2026-10-31)

## Core idea (from research)
Crimson Desert still runs the MMO client/server split from its BDO lineage inside one process, over an unencrypted
in-memory loopback of ~1,115 "Troc" message types (cdmp research, build 2976). So co-op does not need to puppet bones or
fake animations: we **sit on that loopback**. A remote player is spawned the way the MMO spawned other players
(`ActorLoadDataAck`) and driven by forwarding their movement/action messages, so the game animates them natively
(run, jump, glide, climb, ledge pull-ups, combat, skills, effects). Our own snapshots (`PlayerSnapshot`) are the
fallback/sanity channel and feed nameplates, map markers and the party HUD.

## Layers (brief section 7)
```
             +------------------------------- cdcoop.asi -------------------------------+
  game/      | loopback bridge (PostPacket/OnRecvPacket hooks), player reader, actor      |  only layer that
             | spawn/remove, enemy registry (stable ids), stat/loot/quest message writers |  touches the game
  net/       | Steam lobby + ISteamNetworkingMessages, protocol, interpolation, sessions  |
  sim/       | scaling, loot rules, progression merge, config  (pure, unit-tested)        |  no game, no OS
  ui/        | ImGui DX12 overlay: Co-op menu, nameplates, party HUD, debug multipliers;   |
             | native map/minimap markers via game messages                                |
  compat/    | game build check, loaded mods/loaders, content hash, friend-lacks-mod fallbacks |
  core/      | DllMain -> worker thread, logging, paths, fail-safe (any failure = vanilla) |
             +-----------------------------------------------------------------------------+
```
- Host-authoritative: host owns enemies, scaling, loot, progression merges and gameplay settings; guests send intents.
- Signature scanning everywhere, verified per build in `docs/offsets.md`; unknown build = co-op stays off with a message.
- Fail-safe: `safety::disable()` on any hook/scan failure; `cdcoop/VANILLA` or `general.enabled=false` = vanilla mode.
- Loader: Ultimate ASI Loader in `bin64` (as `winmm.dll`), our file `CrimsonDesertCoop.asi`. Runs only in
  `CrimsonDesert.exe` (the loader also enters `crashpad_handler.exe`).
- Modular for a future official multiplayer: everything game-specific is behind `game/`; if Pearl Abyss ships co-op,
  the Comfort Pack and scaling/loot rules can survive while the bridge is retired.

## Milestones (each ends with a two-player test checklist in docs/tests/, STATE note, commit)
| M | Goal | Main work | Depends on |
|---|---|---|---|
| M0 Recon | Loader works; probe logs player position, health, world time; Steam name; build id | `.asi` loads, env report, data-driven probe (done in code), find signatures on current build | GO |
| M1 Ghosts | Steam lobby + invites (overlay "Join Game", invite, rich presence); see each other as markers | lobby/messages via flat API, Hello/version check, overlay markers | M0 |
| M2 Avatars | Native remote player (loopback spawn + message relay), nameplates, map/minimap markers | game/ bridge, ui/ nameplates | M1 |
| M3 Combat | Shared enemies (host-owned, stable ids), damage sync, regular-enemy scaling | enemy registry, guest-native-enemy suppression, scaling apply | M2 |
| M4 Bosses + loot | Boss scaling incl. stagger thresholds; per-player instanced loot, currency/quest items to all | boss stage sync, loot grants | M3 |
| M5 Progression | Shared quest/discovery/map unlock flags; separate co-op save slot; backups | progress flag catalog, save safety | M2 |
| M6 Comfort Pack | Inventory slots, stacks, glide stamina, cooldowns, auto-loot... host-synced toggles | data-table / memory patches behind `comfort.toml` | M1 |
| M7 Release | Packaging, credits, README, Nexus page, changelog | release script (exists), docs | all |

## Testing reality
- Two copies of the game can't run on one PC (author-reported): single-PC tests use an echo/relay peer that plays
  your own character back; real tests need **two PCs and two Steam accounts that own the game** (the user + a friend).
- This cloud session cannot run the game. Phase B needs the user's PC attached to a session (Remote Control) or a
  Claude Code session running on that PC.
