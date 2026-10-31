# STATE (single source of truth for resuming)

**Phase A done. Session 2 (local, on the user's PC) is doing all pre-GO work; the user is asleep (since ~05:00 on
2026-10-31) and asked for autonomous work until at least 11:00. Waiting for "GO: game testing is clear."**

## Where the project lives now
- `C:\Users\asher\CrimsonDesert-SeamlessCoop` (D15; short path because MSVC breaks on deep folders).
- Work branch: `phase-a/local-pc-setup` (pushed after each milestone). `main` = Phase A from session 1.
- Build: `upstream\cd-coop-3446\scripts\build.ps1` (cdmp.asi + tools + tests, RelWithDebInfo) and, for our own
  layers, `cmake -S . -B build\msvc -G Ninja` inside VsDevCmd (PATH needs `...\Microsoft Visual Studio\Installer`).

## Done
- Phase A (session 1): skeleton, sim/net/game/compat/core layers, 42 unit tests, research, D10 build on cdmp.
- Session 2:
  - Game exe 1.0.0.3446 (`D:\SteamLibrary\steamapps\common\Crimson Desert\bin64`), PE timestamp 0x6AE46DD0 =
    upstream `offsets_3446.h`. **Upstream's offline verifier passes every check against this exe**, so upstream's
    addresses hold on the user's current patch.
  - MSVC builds: ours (`CrimsonDesertCoop.asi`, both test suites pass) and upstream (`cdmp.asi`, tools, unit +
    20M-id room codec test pass). **Two-instance loopback test passes** (two real mod instances connect).
  - Nexus permissions verified first-hand (docs/mod-permissions.md, D16).
  - Steam facts, offline: game's `steam_api64.dll` 09.31.86.04 exports everything we use (SteamFriends_v017,
    Matchmaking_v009, Apps_v008, RegisterCallback, overlay invite, rich presence). The exe imports
    `SteamAPI_RunCallbacks` + `SteamAPI_RegisterCallback` (classic dispatch), so callbacks we register will fire.
  - **M1 code: Steam join** (patch 0002, D17): `[net] role=auto`, "Join Game"/invite via callbacks + `+connect_lobby`
    launch argument, Ctrl+F1 invite (Steam overlay), Ctrl+F2 leave, rich presence, role switch between sessions.
    Builds clean, unit-tested parser, loopback still passes. **Needs a live two-PC test.**
  - **Friend-ready package** (`tools/package-coopplus.ps1` -> `dist/CrimsonDesert-SeamlessCoop-<ver>.zip`):
    double-click installer (finds the game via Steam, backs up saves, keeps an existing winmm.dll), uninstaller,
    README, one `role=auto` profile, verified ASI loader v9.7.4, source.zip. Tested on a fake game folder only.
  - Fixed an upstream bug that garbled the UTF-16 ini profiles on Windows checkouts (patch 0003).
  - Test plans: docs/tests/M0.md (single PC, after GO) and docs/tests/M1.md (two PCs, Steam join).
  - **M3 prep: party scaling** (D18): host's enemies take 1/k damage with a guest in (guest blows scaled, host blows
    partly given back). Unit-tested; `[scaling] enabled=0` until a live test.

## Questions for the user (ask when they wake up; work continues meanwhile)
1. Who is the second player for the first two-PC test, and when? (Both need the same mod zip; Denuvo 5 PCs/day.)
2. Hotkeys: Ctrl+F1 = invite, Ctrl+F2 = leave, F8 = panel. OK, or do you prefer others?
3. The GitHub repo is private. GPL needs the source to travel with the binaries: the package includes
   source.zip, so private is fine for friends. Make it public before a Nexus release? (Your call, later.)
4. OK to post a courtesy GitHub issue to nouzert1 (cd-coop-3446) saying we build on the port and offering patches?

## Blockers / waiting on the user
1. **GO** for game testing (another session may be testing Elden Ring on this PC).
2. Two-player tests need a second PC + Steam account that owns the game.

## Next steps (pre-GO, in order)
1. (done) Friend-ready package.
2. (done) Test plans M0/M1.
3. Integration step 2 (our sim/config layers in the same DLL), starting with party-size scaling data plumbing.
4. After GO: M0 + live checks (docs/tests/M0.md), then the two-PC session.
