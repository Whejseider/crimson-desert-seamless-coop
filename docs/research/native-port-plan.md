# Moving Co-op+ onto ByJunter's Native Server (plan, 2026-11-05)

User decision 2026-11-05: "A lot of our problems would be solved with the new repo so I think that is a pretty big
priority for me." Source: https://github.com/ByJunter/CrimsonDesertMultiplayer-NativeServer (GPL-3.0, commit
3cbb03c, cloned to `upstream/cdmp-native/`, working branch `coopplus-3549`; not yet vendored into this repo).

## What it is
The guest's game client logs in to the HOST's internal server (the game's built-in BDO-style server) through a
Steam/UDP tunnel: one shared world. NPCs, schedules, doors, scenes, mounts, pets, fast travel are the engine's own,
for both players. Our current base (cdmp copies + puppets) has to translate each of these by hand; most of our open
issues (missing town NPCs for the host, different town states, doors/levers, quest sharing) come from that.
Its own known gaps: combat latency, time/weather, the world far from the host, shops. Co-op+ already solves
time/weather and the far-away friend in the old base.

## Where it stands
- Built for game 1.0.0.2976 only (5 Steam updates ago). Marked "alpha, not maintained".
- Builds here unchanged (VS 2022 Build Tools): 84/84 unit tests pass.
- About 6 MB of code in `src/mod/session2/` (222 files), much of it diagnostics and research probes.
- Game addresses: 481 named (`constexpr uintptr_t`), 404 code + 134 data/vtables. Searching the recorded byte
  heads in the 3549 exe (build/native-port/heads.py, heads.json): 38 unique, 80 several hits, 115 none (the code
  changed), 248 without recorded bytes (mid-function patch points, vtables, globals).
- No 2976 exe is available (our archive starts at 3479), so the diff tools of the old ports (map_all.py) cannot be
  used directly. The 24 addresses shared with the old base map by name (build/native-port/pairs_offsets.json) and
  show the drift is not uniform (+0x1950 .. +0x1EAD20): no interpolation.

## Plan
1. Two-copy testing on this PC (ByJunter's `scripts/launch-2inst.ps1`: SteamAppId/SteamGameId 3321460,
   PlatformServiceType=Steam, CDMP_ROLE host/guest, cdmp.host.ini / cdmp.guest.ini, the guest's saves in
   `save_cdmp_guest`), merged with our test_guard / savecheck snapshot rules.
2. Port the core only, stage by stage as in its docs/OPCION-D-EMULADOR.md §5: `game/offsets.h` (old-base values by
   name), `session2/offsets_s2.h` (session, user, recipients, allocators, server recv/send), then the login, the
   movement relay, the host-save protection and the guest's thin client. Everything not needed stays off (its
   modules already switch off on a head mismatch).
   Tools: heads.py (byte heads), RTTI (vt.py) for vtables, Troc names for messages, code references for globals,
   disassembly of the new functions for mid-function patch points.
3. First two-copy test of the native session; compare with the current base.
4. Move the Co-op+ layer over (UI and cards, challenges and scoreboard, settings panel, installer, Diagnose,
   consent and save-safety rules); drop what the shared world makes unnecessary (puppets, enemy relay, loot twins,
   live quest copying, co-op worlds).
5. Ask ByJunter (he is in the Discord): is he porting it himself, which modules are essential, any 2976 mapping.
