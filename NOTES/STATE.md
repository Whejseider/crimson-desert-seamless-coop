# STATE (single source of truth for resuming)

## NOW (2026-11-01 afternoon) -- read this first
- Game is **1.0.0.3479** (2.04.05, Steam update 2026-11-01 15:09); the mod targets it only. Package
  `dist/CrimsonDesert-SeamlessCoop-0.1.0-alpha+d56adad8.zip` is built from HEAD, Defender-clean, installed (user's
  cdmp.ini kept). A copy of each game exe is now kept in build/exe-archive (git-ignored) for the next port.
- **Single-PC echo test on 3479 passes:** every hook installs; the other player's character loads, is drawn, stands
  next to ours; **gear changes are shared live** (helmet off/on relayed both ways, the copy follows).
- New since 3460: equipment watcher (reads our equip slots, replaces the dead writer hook); overlay finds Streamline's
  swap chain class by RTTI instead of creating a throwaway swap chain (fixed a ~1-in-5 startup crash with DLSS frame
  generation); name tag 1.95 m. Details: docs/research/port-3479.md, port-3460.md.
- The user is choosing a second player. Not verified yet: anything between two PCs (Steam join/invite live,
  two real players, enemies, hits, scaling), animations in detail, enemy copies' visibility.
- 2026-10-31 20:55 the PC powered off mid-test (Kernel-Power 41, no bugcheck, no dump; the mod was steady). Keep
  game sessions short; watch for it again.
- Dev tools: `[dev] load_trace=1`; test helpers in build/port3460 (trial/keys/shot/mouse/click/run_echo .ps1,
  live.py/actor.py) and build/port3479 (heads/near/vt/globals/troc .py for the next port).
- Upstream issue with our 3460 findings: https://github.com/nouzert1/cd-coop-3446/issues/1

## Where the project lives now
- `C:\Users\asher\CrimsonDesert-SeamlessCoop` (D15; short path because MSVC breaks on deep folders).
- Work branch: `phase-a/local-pc-setup` (pushed after each milestone). `main` = Phase A from session 1.
- Build: `upstream\cd-coop-3446\scripts\build.ps1` (cdmp.asi + tools + tests, RelWithDebInfo; also builds our
  `src/sim` into the mod, D20). Our own layers alone: `cmake -S . -B build\msvc -G Ninja` inside VsDevCmd (PATH
  needs `...\Microsoft Visual Studio\Installer`). Player zip: `tools\package-coopplus.ps1`.
- All Co-op+ changes to upstream code: `upstream/patches/0002-coopplus.patch` (+0001, 0003), see upstream/README.md.

## Done
- Phase A (session 1): skeleton, sim/net/game/compat/core layers, 42 unit tests, research, D10 build on cdmp.
- Session 2 (all offline; the game was never started):
  - Game exe 1.0.0.3446 (`D:\SteamLibrary\steamapps\common\Crimson Desert\bin64`), PE timestamp 0x6AE46DD0 =
    upstream `offsets_3446.h`. **Upstream's offline verifier passes every check against this exe.**
  - MSVC builds of everything; all unit tests pass; **two-instance loopback test passes** after every change.
  - Nexus permissions verified first-hand (docs/mod-permissions.md, D16).
  - Steam, offline: the game's `steam_api64.dll` 09.31.86.04 exports everything we use; the exe pumps
    `SteamAPI_RunCallbacks` itself, so callbacks we register fire.
  - **M1: Steam join** (D17): `[net] role=auto`, "Join Game" / overlay invites (Ctrl+F1), leave (Ctrl+F2), rich
    presence, `+connect_lobby` launch argument, Steam name as the player name. Needs the two-PC test (M1.md).
  - **Package** (`dist/CrimsonDesert-SeamlessCoop-<ver>.zip`): installer (finds the game via Steam, backs up saves,
    keeps an existing winmm.dll), uninstaller, Diagnose (report to Desktop), Vanilla mode on-off, README, one shared
    `role=auto` profile, `cdcoop\config\scaling.toml`, Ultimate ASI Loader v9.7.4 (hash-checked, Defender-clean),
    source.zip. Tested on a fake game folder only.
  - **Party scaling** (D18, D20): with a guest in, the host's enemies take 1/k damage (k from scaling.toml: regular
    x1.5, bosses x1.7). Experimental, `[scaling] enabled=0` until the M3 live test.
  - **Who can join** (D21): `[net] join=friends` (default, "Join Game") or `join=invite` (private lobby, only
    invited friends).
  - **Nameplates**: Steam name over the other player, distance fade; Ctrl+F8 toggles (F8 = panel).
  - **Save guard** (D19): saves copied to Documents when a co-op session starts; warning if the game saves during it.
  - Panel title "Crimson Desert Seamless Co-op+" with the GPL credit line (ByJunter, nouzert1).
  - **Offset audit** (docs/offsets.md): the puppet's wall-climb and mount fixes pointed 0x10 into their functions on
    3446 and would never have installed; fixed and now checked by verify-port.
  - Upstream bugs found and patched: UTF-16 ini profiles garbled by CRLF on Windows (0003); `opcodes_resolved`
    defined in an anonymous namespace (0002).
  - Designs: docs/progression-sync.md (M5), docs/comfort-pack.md (M6; stack-size generator tools/make_comfort_pack.py).
  - Test plans: docs/tests/M0.md (one PC, incl. captures for M5) and docs/tests/M1.md (two PCs).

## Ready for GO
- Package `dist/CrimsonDesert-SeamlessCoop-0.1.0-alpha+22202d0f.zip` (4.2 MB; rebuild with tools\package-coopplus.ps1
  after any commit): built from a committed HEAD, all tests green, verify-port green, Defender clean, installer /
  update / uninstall tested on a fake game folder. Not yet installed in the real game folder (that is Phase B).
- Also new since the first STATE of this session: join/leave toasts; "after a game update" stays off visibly;
  CHANGELOG.md; draft Nexus page (docs/drafts/nexus-page.md).

## Questions for the user (ask when they wake up; work continues meanwhile)
1. Who is the second player for the first two-PC test, and when? (Both need the same zip; Denuvo 5 PCs/day.)
2. Hotkeys: Ctrl+F1 invite, Ctrl+F2 leave, F8 panel, Ctrl+F8 nameplate. OK, or other keys?
3. The GitHub repo is private. The package carries source.zip, so private is fine for friends (GPL). Public before a
   Nexus release? (Later.)
4. OK to post the courtesy GitHub issue to nouzert1 (draft: docs/drafts/upstream-courtesy-issue.md)? It reports their
   UTF-16 ini bug and offers our Steam join code.
5. The GitHub CLI token lacks the `workflow` scope, so CI files cannot be pushed from here. To allow it run
   `gh auth refresh -s workflow` (browser sign-in). Pending CI change: docs/drafts/ci-tool-tests.patch.

## Blockers / waiting on the user
1. GO for game testing: given 2026-10-31.
2. Two-player tests need a second PC + Steam account that owns the game.

## Next steps
1. After GO: docs/tests/M0.md (install the package, load, live addresses, one-PC echo, Steam UI, captures).
2. Then the two-PC session (docs/tests/M1.md) and, with a guest, the M3 scaling check (`[scaling] enabled=1`).
3. M5 tier 1 (discovery/knowledge sharing) once the captures give the payload layouts.
4. Comfort Pack: check CoopPlus-Stacks with CDUMM's Inspect Mod; add inventory/cooldown/glide fields.
5. Pre-GO work still possible: more offline review of the new code paths; keep the package current.
