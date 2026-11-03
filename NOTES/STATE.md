# STATE (single source of truth for resuming)

## NOW (2026-11-01 late) -- read this first
- FIRST read NOTES/HANDOFF-2026-11-01.md (test kit; its "OPEN BUG" section is now RESOLVED-ish, see its top note).
- Work from NOTES/PLAN.md (user's standing instruction: plan, then carry it out without stopping to ask).
- Game **1.0.0.3479**. verify-port ALL PASS (run from upstream/cd-coop-3446 with -Exe "D:\SteamLibrary\...\CrimsonDesert.exe");
  tools/port/check_opcodes.py build/port3479/troc_3479.txt -> all match.
- Installed: dist/CrimsonDesert-SeamlessCoop-0.1.0-alpha+059edd1c.zip (Defender clean, user cdmp.ini kept). Test save OUT.
- Death blocker (A12): once the other player's copy has been loaded, the game never shows its death screen again
  (bisect in LOG). Co-op+ now shows its own Q/U box (net/revive_prompt.*, D26): Q verified; U = full health at the
  checkpoint server-side, client moved there only sometimes. Relay table + AttackByCollisionReq were stale (D27): fixed.
- 2026-11-02: A9 boss-fight deaths built (D29), party scaling on by default (D28), boss HP now reaches boss_safe.
  After EVERY test session run build/port3479/savecheck.sh (SAVES OK); savewatch.sh logs save-folder writes.
- 2026-11-02 (later): new user rule (NOTES/FLAGS.md: work everything doable, flag the rest, never ask "continue?").
  Backlog docs/COOP-FEATURES.md + research docs/research/*. Done on one PC: horse riding together (D30, default on),
  shared waypoints + Ctrl+F3 ping, Ctrl+F4 go-to-friend and a reliable checkpoint revive via hopped DoTeleportAck
  (D31), save guard sees server autosaves (slot 2). Flags F2 (flying needs a late save), F3 (cutscene capture),
  F4 (minigame captures), F5 (info: autosave into slot2; test kit restores it: savecheck.sh --fix).
- 2026-11-02 (afternoon, user: "complete everything; make test saves yourself"): save format cracked (tools/save,
  docs/research/save-format.md); dragon test save (build/test-saves/slot103-dragon, testsave.ps1 -Variant dragon):
  Blackstar is summoned from the F1 wheel and the friend's side gets a flying dragon copy; riding it not reached.
  Cutscene flag exe+0x70D2E2F (D32: notice + shared skip, verified: skip_test pressed Esc in our cutscene). Arm
  wrestling captured; minigame notices (D33). Long hops (D34). Dev script Ctrl+Shift+F6 ([dev] script=file: c2s/s2c/
  wait/tp). Test helpers: travel_test.sh (TO=x,y,z), act_test.sh (pub arm wrestling; needs game time ~10:30, i.e.
  within ~2 real minutes of loading), dragon_ride.sh, mount_probe.sh, stage_test.sh, npcs.py, heapscan.py,
  gaugehunt.py, wheel.ps1 (F1 wheel; CURSOR:x,y selects), cutflag.py.
- Verified on one PC (echo) earlier: friend's character, gear, name tags, shared enemies (copies, hits, aggro,
  hiding), all on by default. Open: loot pickup (two PCs), bosses (A6/A8/A9), riding, Steam checks, playtest docs
  (docs/tests/PLAYTEST-1.md drafted). Test save "COOP TEST bandits" is IN as slot103 -- take it Out before handing over.
- Test helpers: build/port3479/kill_test.sh (deterministic death, Ctrl+Shift+F9 via [dev] kill_key=1, REVIVE=Q|U,
  WALK_X/WALK_Z), death_test.sh (bandit fight; NOATTACK=1), bandit_test.sh, walkto.py, reqsizes.py, loads.py.

- 2026-11-02 (evening, user: minigames vs friends, wyvern/griffin flight, time + weather MUST match, boss fights with
  their cutscenes, skip = everyone presses): Installed dist/CrimsonDesert-SeamlessCoop-0.1.0-alpha+aff91978.zip
  (Defender clean, cdmp.ini kept, DLL hash matches). Test save OUT, shipped ini restored, SAVES OK.
  Done: everyone-must-skip (D32 rev.), versus arm/hand wrestling (D35, only gauge games), time sync on by default
  (D36), WEATHER sync (D37: weather = random server stages; guest runs the host's; verified with rain), boss/story
  cutscene relay (D38: the host's stage copied into the guest's client; safe on one PC, plays only provable on 2 PCs).
  Flags: F3 (boss cutscene needs a 2-PC boss test), F7 (wyvern: egg test save ready, how does it hatch?).
  Open (mine): RPS versus (boy busy at 10:35-10:45; design in LOG), other contests' scores.

- 2026-11-02 (night): CHALLENGES (D40): Shift+F5 challenges the friend to arm/hand wrestling at a table or to a
  rock-paper-scissors duel anywhere (D39: the town RPS child cannot be made PvP); Shift+F5 accept/end, Shift+F6
  decline; series score. Verified on one PC with [dev] challenge_test / versus_test / rps_test. Installed
  dist/...-0.1.0-alpha+afd35141.zip (Defender clean, DLL hash matches, ini kept). Test save OUT, ini restored,
  SAVES OK. Wyvern still F7 (needs a save that reached Delesyia). Test-save tools: move_save.py, knowledge_swap.py,
  make_egg_save.py; variants rps, windridge, wyvern-egg, wyvern-k(-main).

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
