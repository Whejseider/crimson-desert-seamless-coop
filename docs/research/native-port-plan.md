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

## ByJunter's notes (2026-11-05, relayed by the user)
- ~530 hardcoded RVAs across 60 files; he would use signature scans from day one (porting is the biggest cost).
  Our plan: port by hand to 3549 now, then move the ported addresses behind a signature/AOB resolver so the next
  game patch is a re-scan, not a re-port.
- Denuvo: much of the logic lives in the mutated section (.xtls on 2976, .xpdata on 3549). Read it, never hook it.
  Hook function entries in the normal code section (with .pdata), or safetyhook mid hooks there.
- One new user per field: ServerField::AddActor rejects a second new user per field, so field_share is required
  for the guest to enter the host's field.
- NPC controllers: NPCs spawned for the guest's requests (schedules, road travellers, carts) were controlled by the
  host's client and stood still; they must be assigned to the guest's client, a whole family / caravan at once.
- Host scenes on the guest: cutscenes and stages took over the guest's camera, UI, controls (guest_seq_foreign_*);
  doors needed the guest's scene level shared (scene_level_share).
- Overlay: inline hooks only (Streamline calls Present directly), installed late (WorldSystem + visible window).
- Never identify entities across PCs by address: actor keys. Always clean builds (a stale object once read the
  config at wrong offsets). Two copies on one PC: scripts/launch-2inst.ps1.
- Unsolved upstream: shops, enemies with carts, some dead zones far from the host, destroyed props not respawning.

## 3549 findings so far (layout changes, not only addresses)
- TransformSync: FieldMover at TS+0x508 (2976: +0x4F0); FieldTime still TS+0x450, actor TS+8.
- FieldMover: field FM+0x10 and key copy FM+8 unchanged; the busy counter is FM+0x4A (2976: +0x5A).
- ServerField: owner +0x180, open +0x4A, lock +0x18 unchanged; key +0x2D0 (2976: +0x2C8); level entry +0x2D8
  (2976: +0x2D0).
- Inlined on 3549: FieldMover::SyncFieldTimeOnMove (clock_gate now a mid hook on its owner check).
