# Log (append-only)

## 2026-10-31 (session 1, cloud container, Phase A)
- 07:58 Read brief (PDF). Environment: Linux cloud container, NOT the user's game PC. No Chrome/computer-use tools in this session. GitHub scope: ashersheets-jpg/elden-force-source, branch claude/seamless-coop-mod-t4lqdd.
- Created project folder `CrimsonDesert-SeamlessCoop/` in that repo (D1).
- Toolchain: cmake 3.28, ninja, g++ 13, clang, lld-link present. `apt-get install mingw-w64` -> x86_64-w64-mingw32-g++-posix 13 (OK).
- Network: www.nexusmods.com, api.nexusmods.com, old.reddit.com -> proxy 403 / DNS fail (blocked by environment network policy). WebSearch works. raw.githubusercontent.com and git clone from github.com work; api.github.com is scoped to the session's repo.
- Vendored toml++ v3.4.0 (MIT) from raw.githubusercontent.com.
- Spawned 3 research agents (engine/DRM/loaders; prototype/news/other co-op mods; Nexus survey). News sites also unreachable for them (DNS) -> news content from search snippets only (medium confidence).
- Wrote sim/ (scaling, loot, progression, config), net/ (protocol, interpolation, steam_runtime), game/ (sigscan, probe_spec, probe), compat/, core/ (dllmain, log, paths, safety), 8 test files, 6 TOML configs.
- Build: first error `toml::parse(string_view, const char[])` ambiguous -> pass std::string_view. Tests: 41/42; config test caught a real bug (`guest_progress_counts_for_host` placed under `[shared]` in progression.toml) -> moved above the table. 42/42 pass.
- MinGW cross-build: CrimsonDesertCoop.asi PE32+ DLL, imports KERNEL32/msvcrt/VERSION only, exports InitializeASI. Stripped ~1.3 MB.
- Added .github/workflows/cdcoop.yml (MSVC build + tests + artifact; Linux tests + MinGW build), tools/release.sh (build, test, zip) -> dist/CrimsonDesert-SeamlessCoop-0.1.0.zip OK.
- Nexus survey done (search snippets only; all permissions UNVERIFIED). Game now at patch 2.04.00 (~Oct 30).
- Prototype research done. Big find: ByJunter's cdmp (GPL-3, C++20 ASI, abandoned, invites forks) + nouzert1/cd-coop-3446 (port to build 3446, last commit today). Cloned both read-only to /tmp/claude-0/ext/ (outside the project), read README/ROADMAP/research index/CONTRIBUTING -> NOTES/RESEARCH/cdmp-codebase.md.
- Wrote CLAIMS.md, docs/offsets.md, docs/protocol.md, docs/architecture.md, docs/tests/M0.md, DECISIONS D1-D9.
- Engine/DRM research done: Denuvo Anti-Tamper (5 PCs/day) + Themida-style protector (.xtls); no anti-cheat; ASI mods hooking at function entry work; launch must go through Steam. Loader: Ultimate ASI Loader 9.7.x as winmm.dll (winhttp.dll fallback after Win11 KB5095093). Builds: 2.04.00 = exe 1.0.0.3392 -> 3424 -> 3446.
- Asked user (AskUserQuestion): build on cdmp? -> YES. Phase B how? -> attach PC via Remote Control.
- list_computer_folders -> no folders shared yet.
- git push -> 403 (Claude GitHub App lacks access to the repo). add_repo push says already attached; retry still 403. Commits kept locally.
- Imported cd-coop-3446 b7ae133 via `git archive` (rsync not installed) into upstream/, minus bundled dinput8.dll. LICENSE -> GPL-3.0.
- Upstream tests on GCC: error in enemy_zone.h (default arg `Settings s = {}` with nested default member initializers; GCC/Clang reject, MSVC accepts). Patched (two ctors), saved as upstream/patches/0001. Upstream suite: "All tests passed" on Linux; also cross-builds with MinGW.
- Wrote docs/integration-plan.md, CREDITS.md, README.md, D10-D12; CI job for upstream MSVC build.
- User: Crimson Desert installed via Steam on D: drive. list_computer_folders still empty (PC not attached yet). Recorded in STATE.
- User reconnected GitHub and asked for a fully separate repo (nothing in Elden Force). create_repository -> 403 "Resource not accessible by integration"; list_repos finds no crimson repo. Split history with `git subtree split --prefix=CrimsonDesert-SeamlessCoop` into /home/user/crimson-desert-seamless-coop (main, 4 commits, no Elden Force history). Moved CI workflow to .github/workflows/build.yml (root paths), fixed patch paths (-p1). Reset the local elden-force-source branch to c48f916 (its original state; nothing had been pushed).
- User created the repo via the browser; it then appeared to the GitHub App. add_repo OK, remote was empty, pushed main (02ca593). Repo private.
- User turned on Remote Control. list_computer_folders: DESKTOP-SM0R9J3 / JumpRing in cannot_be_used (Windows computers can't serve folders to cloud sessions). Not using JumpRing (Elden Ring). Proposed local session on the PC (D14).

## 2026-10-31 (session 2, local Claude Code on the user's Windows PC)
- Cloned the repo into a folder the app made for this session (very long path). README_FOR_CLAUDE -> brief -> STATE -> LOG.
- Game exe read-only: D:\SteamLibrary\steamapps\common\Crimson Desert\bin64\CrimsonDesert.exe, FileVersion 1.0.0.3446,
  332786072 bytes. Not launched.
- Toolchain: vswhere finds VS Build Tools 2022 17.14.37710 (MSVC 14.44.35207); CMake 3.31 + Ninja bundled under
  Common7\IDE\CommonExtensions\Microsoft\CMake; Windows SDK 10.0.22621 and 10.0.26100. cmake/cl not on PATH (use VsDevCmd).
- MSVC attempt 1 (build dir in the repo): VsDevCmd warned vswhere missing from PATH; CMake try_compile failed (path too long).
- MSVC attempt 2 (build dir %TEMP%\cdcoop-b): our core, DLL and test objects compiled (warnings only: C4996 _wfopen,
  C4127 in upstream tests). Failed: C1083 cannot open upstream\cd-coop-3446\src\common\client_session.cpp, because the
  full source path is over MAX_PATH. Decision D15: move the project to C:\Users\asher\CrimsonDesert-SeamlessCoop.
- User sent the brief PDF (same as docs/BRIEF.txt) and preferences: contact only at pivotal moments; Chrome is signed in
  to Nexus/GitHub for our use.
- Nexus permissions: own Chrome tab, fetched each mod page (read-only GET, 0.7 s apart) and parsed the "Permissions and
  credits" section. 26/27 read; 3380 CDAutoLoot hidden since 2026-09-13 (unsupported). docs/mod-permissions.md updated.
  Tab closed.
- gh api: ThirteenAG/Ultimate-ASI-Loader licence MIT, latest release v10.0.2 (2026-10-27).
- 04:45 Moved to C:\Users\asher\CrimsonDesert-SeamlessCoop (app did not copy; cloned branch phase-a/local-pc-setup there).
- MSVC (Ninja, Release) of ours: CrimsonDesertCoop.asi + cdcoop_tests + upstream_tests, 2/2 ctest pass. Warnings:
  C4701 protocol.cpp:102 'reason' maybe uninitialised, C4996 _wfopen (log.cpp).
- Upstream own CMake (RelWithDebInfo): cdmp.asi, cdmp_peer/host/probe/dx12_test/tests; unit + roomcodec pass.
- verify-port.ps1 -Exe D:\...\CrimsonDesert.exe: ALL OFFLINE CHECKS PASSED (PE timestamp 0x6AE46DD0, image size
  0x14268000, all measured globals, prologues of the send/recv/minimap/equip hooks, the 4 containment sites, RTTI).
- e2e-loopback.ps1: PASS (handshake, build ids, state both ways, no rejects, clean shutdown, build-id self-test).
- User (05:00): going to bed; work autonomously until at least 11:00, push after milestones, keep a question list.
- dumpbin: steam_api64.dll 09.31.86.04 exports SteamAPI_RegisterCallback, ActivateGameOverlayInviteDialog,
  SetRichPresence, SteamApps_v008 GetLaunchCommandLine, Matchmaking_v009 lobby member calls. Exe imports
  SteamAPI_RunCallbacks/RegisterCallback/UnregisterCallback (no ManualDispatch).
- Wrote steam_join.{h,cpp}, join_args.h, run_coop/apply_role_settings in mod.cpp, role=auto, HUD hint, tests.
  Build: no warnings in changed files; unit tests (incl. test_join_args) pass; loopback passes. Patch 0002 recorded.
- 05:10 Found upstream bug: upstream .gitattributes `kit/** text eol=crlf` corrupts the UTF-16 cdmp.ini.* profiles on a
  Windows checkout (each LF gains a lone 0x0D). Exempted them (patch 0003); blobs were fine.
- Ultimate ASI Loader v9.7.4 NoPDB_x64 from GitHub: SHA-256 matches GitHub's digest; Authenticode "FusionFix"
  self-signed (UnknownError = untrusted root, expected); Defender custom scan: no detections.
- Steam name: mod uses GetPersonaName when [general] player_name is empty/"Player" (protocol caps 32 bytes, ASCII).
- tools/package-coopplus.ps1 -> dist/CrimsonDesert-SeamlessCoop-0.1.0-alpha+f815d26f.zip (4 MB). First run failed:
  PowerShell function `Git` shadowed git.exe (case-insensitive) -> recursion; renamed Invoke-Git.
- Installer test on a fake bin64/save in %TEMP%: backup OK, existing winmm.dll kept and restored, update keeps
  cdmp.ini (writes cdmp.ini.new), uninstall removes exactly the listed files. GetPrivateProfileString reads the
  generated UTF-16 profile: role=auto, transport=steam, coop=1. Find-CrimsonDesertBin64 finds D:\SteamLibrary\...
- 05:20 Test plans: docs/tests/M0.md rewritten for the cd-coop-3446 base; new docs/tests/M1.md (two-PC Steam join).
- Upstream on GitHub: still b7ae133 (our import is current). Their 2026-10-31 two-PC script says the core question
  (does the guest SEE the host's character) is still open; 270e2cc position fix not yet verified live.
- Party scaling (D18): common/party_scaling.h (pure, tests), net/party_scaling.*, hooks in local_capture (VaryHPAck)
  and guest_hits (scale + ledger), [scaling] enabled=0 health_2p=1.7, panel line, 5 s log line. heal_back first
  computed loss*(1-1/k) -> 17 HP gave 6 back with k=1.7f (float); now loss - round(loss/k), matching guest blows.
  Build clean, all unit tests pass, loopback passes, root MSVC build + ctest pass. Our C4701 (protocol.cpp) fixed.
- Self-review of run_coop: join callbacks now registered during the Steam-name wait; a role change always
  reconfigures the guest's enemy zone (a role=guest player who left kept hiding enemies as host). Build/tests/loopback OK.
- 05:29 Nameplates (brief 6.2): common/nameplate.h fade (full to 25 m, 35% at 200 m, hidden from 600 m), text shrinks
  to 80%; F8 now toggles only the panel, Ctrl+F8 the nameplate; [overlay] nameplates=1. Panel moved into draw_panel().
  Occlusion not done (needs the depth buffer; live work). Build + tests OK. Visual check needs GO.
- Diagnose Co-op+.cmd (diagnose.ps1): report to the Desktop (versions, mod files + hashes, ini settings, log tail);
  tested on a fake install with -OutDir. Comfort Pack plan in docs/comfort-pack.md (CDUMM .field.json, companions
  now, our own field.json after GO). README rewritten. Draft courtesy issue in docs/drafts/ (not posted).
- 05:36 Save guard (D19): save_guard.{h,cpp}, hook in capture_observe_c2s, backup on connect, panel notice.
  First link failed: upstream declares cdmp::net::opcodes_resolved() but defined it inside the anonymous namespace
  (never called before) -> moved out. Upstream's opcode_for comment says "TrocTrMoveActorReq" but names are stored
  without the TrocTr prefix (call sites use "MoveActorReq"); comment fixed, our names "SaveAutoReq" /
  "SaveToFileCurrentPlayerReq" (2976 table: 3037 / 2910 as fallbacks; opcodes checked equal 2976 vs 3446 for
  VaryHPAck 2147, DoFrameEventReq 2380, VaryStatCheatReq 2703, NotifyHandleDeadAck 2530). Build/tests/loopback OK;
  no backup folder created by the loopback (co-op hooks only run in the game).
- 05:37 Panel title: Co-op+ with a GPL credit line (cdmp by ByJunter, port by nouzert1). docs/progression-sync.md (M5
  design: discovery/knowledge first via ShareKnowledgeDataListReq / LearnKnowledgeByCheatReq, quests last and opt-in).
  M0 gains an optional capture step for M5 payloads.
- Vanilla mode on-off.cmd (vanilla.ps1): parks cdmp.asi as cdmp.asi.off; install turns it back on, uninstall removes
  it. Tested on a fake bin64: install, toggle x2, vanilla+reinstall, vanilla+uninstall all correct.
- 05:43 D20: coopplus_sim library in upstream CMake; scaling per tier (bosses via boss_is_boss); scaling.toml read
  from bin64\cdcoop\config. cdmp_host test with health_2p=1.3 in the ini and scaling.toml present: log 'regular x1.50,
  bosses x1.70', i.e. the TOML curves win and the ini switch turns it on. Package ships cdcoop\config\scaling.toml;
  installer keeps a player's edited copy on update (.new), uninstaller removes the folder when empty.
- Package 0.1.0-alpha+5f39326b built (4.2 MB) and tested on a fake bin64: install, update with an edited
  scaling.toml (kept, .new written), uninstall (only the exe left). Root ctest 2/2 pass.
- 05:45 Comfort Pack generator tools/make_comfort_pack.py (stacks via Format 3 match on iteminfo max_stack_count,
  field name from CDUMM's public item writer), 4 unittest cases pass; CI runs them. Untested in the game.
- Push refused: token scopes gist/read:org/repo, no 'workflow'; GitHub rejects commits touching .github/workflows.
  Rewrote the unpushed commit without the CI change (saved as docs/drafts/ci-tool-tests.patch); question added.
- 05:50 D21 invite-only option; build clean, tests + loopback pass. Patch apply check: 0001+0002+0003 apply to the
  pristine import (cb21b60) and reproduce src/ exactly.
- 05:51 Self-review of the mod.cpp/steam_join/room_code diff: session end now clears rich presence on every end
  (a Join left a stale 'Join Game' on the old lobby) and drops party scaling to 1 player right away. Locking reviewed:
  callbacks lock-free, fail() never re-locks. Build/tests/loopback OK.
- Review: scaling ledger now records guest blows whenever scaling is active (a boss-only multiplier would have
  scaled a guest's boss blow twice). save_guard reviewed, no change.
- 05:57 OFFSET AUDIT: kPointClimbEnter/kRideOnEnter/kRideOffEnter were 0x10 too high on 3446 (RTTI vtables slot
  +0x38 hold 0x75F620/0x74E8D0/0x74F8A0, whose bytes match the expected heads; the old values point mid-function).
  Fixed + added to verify-port (PASS). Other stale addresses fail closed (error logger, aggro_test, rope dev) - see
  docs/offsets.md audit table.
- Audit cont.: every head array in src/mod is now covered by verify-port or belongs to a dev feature that is off by
  default; values inherited from 3424 are field offsets/opcodes except kServerDockingWriter (compiled out) and the
  camera block offsets (unverifiable offline; added as an explicit M0 check because nameplates depend on it).
- Camera block: only 2 plain-code xrefs to the camera global (0x2FB8997 reads +0xC8/+0xD0; 0x3E1C8D3 writes it);
  the +0x428 block access is not in plain code. Not determinable offline; M0 check stays.
- 06:01 Join/leave toasts (top centre, independent of F8). Build/tests/loopback OK.
- 06:02 Unsupported game build (any PE timestamp but 0x6AE46DD0, in the game only): networking not started (no lobby,
  no Join Game), panel shows a red 'Co-op+ is off: made for 1.0.0.3446, not X' line. README section added.
- 06:04 Correction: entries above stamped 06:20 ... 08:45 were estimates written without checking the clock; they are
  now the real commit times (git log). Times in this log come from `date` / commit times from here on.
- 06:03 CHANGELOG.md (in the package) and docs/drafts/nexus-page.md (M7 draft, not posted).
- 06:04 docs/architecture.md and integration-plan.md: 'how it is actually built today' sections for future sessions.
- 06:05 Final package 0.1.0-alpha+22202d0f built from HEAD; fake-folder install/uninstall OK; ini reads role=auto,
  join=friends, transport=steam, coop=1; Defender: no detections.

## 2026-10-31 11:01 — upstream watch ended
- Polled nouzert1/cd-coop-3446 every 5 min until 11:00: no new commits (still b7ae1337). Nothing to integrate;
  package 0.1.0-alpha+22202d0f stays current. Waiting for "GO: game testing is clear." to start M0.

## 2026-10-31 ~14:10-14:45 — GO received; game had updated to 1.0.0.3460; port done
- User said "GO: game testing is clear". Installer run on the real game folder: worked (game found via
  Steam, 15 save files backed up to Documents\CrimsonDesert-CoopPlus-SaveBackups\2026-10-31_14-13-51).
- The exe was 1.0.0.3460 (Steam update 08:54). Ported every address offline (D22): offsets_3460.h, horse_diag sites,
  verify-port retargeted -> ALL OFFLINE CHECKS PASSED on 3460. Build + unit tests + loopback e2e green.
- Live (read-only probe, vanilla game at the title screen): Troc handlers 1175 at 0x6E86D10..0x6EBA370 (3446 +0x4000),
  clock flag 0x70D1DB5 = 1, WorldSystem/Camera slots non-null.
- Launch 1 with the mod: the game closed ~2 s after start, no crash dialog, empty sentry event, log ended after
  "relay network status 2". Vanilla (loader only): stays up. Mod with net off: stays up and EVERY hook installs
  (sniffer 1175 types, PostPacket/OnRecv, wall-climb fix, horse containment, action-layer guard, minimap icon). udp:
  up. Steam (as shipped) twice more with the new crash logger: up, relay 100, overlay drawing, no exceptions logged.
  Launch 1 stays unexplained (first start after the update?); watch for it.
- Expected WRNs: "Game errors: logger not recognised" (stale since 3446) and "OtherEquipItemAck writer not recognised"
  (not ported, D22).
- No desktop computer-use tools in this session (only browser tools), so loading a save / in-world checks are next.

## 2026-10-31 ~14:45-15:25 — in-game testing on 3460
- In world: local position exact, camera block found at +0x430 (was the panel's "sentinel mismatch"), nameplate
  draws. Echo test: copy spawned 12 km off -> MoveData reordered in 3460 (found via the client parser, see
  docs/research/port-3460.md) and load transform at end-158 -> copy now spawns in place with name plate.
- Movement events never left the PC: every kOp* constant was 2976-era; regenerated all 95 from the live table
  (docs/research/troc-opcodes-3460.txt). Events now flow (225+), injected MoveActorAcks verified byte by byte.
- Copy still not drawn/moving: its player actor stops at load state 0x08 (body and user are fine). Blocker.
- Game patch dialog "Extra Rewards / all skills reset" appears on load (the game's own 2.04.03 change); confirmed
  it in tests, but nothing was saved, so the user will see it again on their own save.

## 2026-10-31 — 3460 findings posted upstream
- With the user's OK, posted docs/drafts/upstream-3460-findings.md (+ full 3460 Troc table) as
  https://github.com/nouzert1/cd-coop-3446/issues/1. Watch it for a reply about the 0x08 -> 0x10 load step.

## 2026-10-31 evening — copy load fixed; copy not drawn yet
- Dev load trace found the copy stuck at load state 8 because HorseDiag's containment skipped
  ClientCharacterControlActorComponent vf 0x80 (= the loader's "still loading?" query) with a bare ret. Fixed
  (commit 09b498a): copy reaches 0x10, injected moves apply, it follows 2 m away (verified in memory + panel).
- Spawning the copy while our own world is still loading drops our control mode 2 -> 1 and the game stays on its
  loading scene; spawned after our world is up, all normal. TODO: hold the spawn until we are in control.
- Copy still not drawn: name plate floats over empty ground at its position (camera turned to check). Same 48/23
  components as ours; status flags +0x390/+0x394 are 0. Visibility path in 3460 is the next investigation.
- Game closed without saving after every test; shipped cdmp.ini restored; saves still dated 2026-05-13.

## 2026-10-31 ~18:00-18:40 — the copy is drawn
- Loading detector opcodes renumbered (2294/2190): the copy now loads after our world; control intact.
- Invisible copy: real hide flags at status +0x3B8/+0x3C0; copy keeps bit 20. puppet_visibility clears it via the
  game's handler exe+0x6E90A0 from the copy's movement step. Copy visible and following in the echo test.
- Package 0.1.0-alpha+219050be built, Defender clean, installed. Saves untouched (still 2026-05-13).

## 2026-10-31 ~20:00-20:55 -- overlay crash fixed; equipment watcher
- A launch closed the game ~14 s in: crash log showed sl.dlss_g.dll / sl.interposer.dll faults right as the overlay
  created its throwaway D3D12 swap chain (also explains the 14:1x "launch 1"). A D3D11 dummy still crashed 1/5.
  Fix: read Streamline's proxy swap chain vtable from RTTI (no swap chain created): 6/6 clean, same hook addresses.
- The upstream equip-writer site is a log formatter on 3460 (41 identical ones). New equipment watcher reads our
  ClientEquipSlotActorComponent (comp+0xA0 slot table, static item table 0x71B6C20) every 250 ms. Read 17 slots.
- 20:55 the PC powered off during an echo test (Kernel-Power 41, no bugcheck/dump). Saves untouched.

## 2026-11-01 15:00-15:30 -- port to 1.0.0.3479, gear changes verified
- PC back up 15:02; Steam updated the game to 1.0.0.3479 at 15:09; the mod switched itself off as designed.
- Ported (docs/research/port-3479.md): small patch, code +0..-0x70, vtables/Troc +0x1000, data +0xFB0..+0xFC0,
  opcodes unchanged. verify-port ALL PASS, loopback e2e PASS.
- Echo test on 3479: all hooks in, copy drawn. Inventory test: helmet off -> copy loses it; helmet on -> copy wears
  it again. Game closed without saving; saves still 2026-05-13.
- Package 0.1.0-alpha+d56adad8 built, Defender clean, installed (cdmp.ini kept).

## 2026-11-01 15:35-16:05 -- enemy-copy attempt, menus, Lootr request
- User asked for Lootr-style loot (everyone takes every item for themselves): recorded as D24; it is the brief's
  default and the per-player worlds already give it for chests/world items; the loot twin covers enemy drops.
- Enemy copies: none can be made at the save spot (0 enemy loads); auto-walk toward the bandit camp fell off a cliff
  (character died, game closed, nothing saved). Deferred to the two-PC test / a manual session.
- Name tags showed over the world map: found the game's gameplay-input flag (input-mode push 0xF20AC7 / 0x71982B8);
  tags now hide over every full-screen menu. Verified live.
- Package 0.1.0-alpha+6e2b1940 built, Defender clean, installed (cdmp.ini kept). Saves still 2026-05-13.

## 2026-11-01 16:10-17:15 -- enemy copies: two fixes, one blocker
- With the user's OK, made a test save next to enemies: fast travel to Hernand Town (Abyss Nexus), walked to Rodbel
  Fishing Dock (Bleed Bandits; one death fall on the way, instant revive), saved into the EMPTY slot only (slot103
  "COOP TEST bandits"), later moved it out to build/test-saves. User's 12 save files equal to the 16:04 backup.
- Fixed: no enemy ever passed the zone test on 3460+ (T0 +4); the game clock never decided (bad bound check), so
  copies waited forever. Added CC per-frame containment (0x944080).
- Blocker: a bandit copy loads, then crashes the game in its load-complete path (0x976921, null action). Enemy copies
  gated off by default ([dev] enemy_copies=1); verified refused + no crash.
- Package 0.1.0-alpha+801f3f51 built, Defender clean, installed (cdmp.ini kept).

## 2026-11-01 17:15-19:55 -- shared combat working on 3479
- Plan written (NOTES/PLAN.md) per the user's instruction; user added: bosses scale to 4 players, boss-fight respawn
  rule, no friendly fire, explain enemy targeting.
- Enemy copies: game error logger re-found (0x15FAF40) and it showed unpack failures; 3 crash paths of actionless
  copies guarded (CC tick 0x944080, animation update 0x975B60, late update 0x94EA00). 6 copies through a fight, no crash.
- DoFrameEventReq on 3479: entries 677 B, accessor 0x1FB -> guest hits on copies kill the host's enemies.
- Loot twins: SpawnCharacterCheatReq now 22 B; both forms tried logged the player out -> replaced by the friend's own
  hidden native as stand-in (pickup untested on one PC).
- Suppress lab on 3479: 25 natives hidden, all restored. Defaults: enemy_copies=1, suppress=1.
- Aggro ported (IsTargetable 0x23E4C40, SetCurrentTarget 0x23E84C0, CheckAIBattleType 0x24BE2F0): enemies target the
  friend's copy and its blows hurt the friend; defaults on with reactions and lethal blows.
- Packages 6e2b1940 -> 801f3f51 -> bf1b4122; saves verified equal to backup after the test save was made in an empty slot.


## 2026-11-01 21:00-23:55 -- death screen blocker: cause narrowed, revive prompt, stale opcodes
- Found while reading: kRelayedAcks still had 3424 opcodes on 3460/3479 (2530 = AutoSpawnRoadReq now, 3053 =
  ReviveAck...), and player_hits/piece4 sent 2815 (now AcceptJoinGuildMemberReq) as AttackByCollisionReq (2150, 44 B
  checked with reqsizes). Fixed; tools/port/check_opcodes.py added (catches both on the old code).
- Bisect with new test kit (build/port3479/death_test.sh, kill_test.sh; `[dev] kill_key=1` = Ctrl+Shift+F9 kills us
  the way a lethal relayed blow does; `[dev] no_copy=1` never loads the copy):
  * connected, every shipping feature on, copy never loaded -> death screen (bandit death).
  * copy loaded, removed (body+player+user) 1.5 s after death -> no screen. Removed as our NotifyHandleDeadAck
    arrives (before the game handles it) -> no screen. Removed 5 s before the kill, with/without its user, with no
    replayed events at all -> no screen. No peer, kill key -> screen (kill key is a valid stand-in).
  => loading the copy once leaves state that keeps the screen away for the rest of the session.
- Death screen class: UIGamePlayControlReviveModal (uiCommonScript), created by name from UI tables; trigger not found.
- Captures (no peer): Instant Revive = ReviveReq u32 player | 00 01 00 00. Checkpoint = ReviveLoadingStartReq
  04 02 00, +1 s ReviveLoadingReq 02, client GameLoadingStartReq, ReviveReq type 2; server answers
  LoadingFieldDataAck (+28: checkpoint position/rotation/scale) and reloads the field.
- New net/revive_prompt: overlay box after 4 s dead (connected, copy loaded once, no menu). Q verified live (gets up
  at ~110 hp, copy stays). U: server revive at checkpoint verified (full hp); the client ignores the field reload
  outside its own flow; DoTeleportAck to the checkpoint took once (1/4). Client accepts ReviveLoadingStartAck only
  for the type at status+0x471 (16 = none, handler exe+0x6F33D0); setting it is not enough for the loading screen.
- Crash seen in a throwaway experiment: a copy load and its removal in the same instant (exe+0xB6290E). The final
  code never removes the copy around deaths.
- Saves: tests never saved; the shipped cdmp.ini restored after the tests. Test save still IN as slot103.
- SAVE INCIDENT: the user's own slot2 (their 2026-05-13 save, unchanged in every backup up to 19:51) was rewritten
  at 21:22:53 during a test round (earlyout2: echo session on the test save, mid-fight; no SaveAutoReq /
  SaveToFileCurrentPlayerReq seen, no save event in cdmp.log, so the save guard did not warn). Restored from the
  16:04 backup at 23:55; all user save files verified equal to it. The rewritten version is kept in
  Documents\CrimsonDesert-CoopPlus-SaveBackups\overwritten-slot2-by-test-2026-11-01_21-22. Cause unknown (a game
  write path the guard does not see, or Steam Cloud). From now on: run the save check after EVERY test session.

## 2026-11-02 00:00-00:40 -- A6 review, A9 built, A8 scaling on
- A6: boss safety paths use 3479 opcodes (SetAvoidByCutSceneAck 2708, VaryHPAck 2740: match the live table). Found
  boss_note_hp never called: the guest's boss bar and any "fight in progress" state never worked. Fed from VaryHPAck.
  No boss near the test save: the live boss check stays for the two-PC test.
- A9 (D29): tests a9-wait2 (wait box, then normal box 28 s after the last boss HP change), a9-alldown ("Everyone is
  down", U revived at full health). The copy now falls dead beside us when we die (relay table fix): A12 "copy
  plays the death" seen.
- A8 (D28): a8-scaling: 11 give-backs, 56 HP, 12 relayed blows skipped, 0 failed; default on.
- New dev switches: [dev] boss_hp=N, partner_alive=1, scaling_as_host=1. Test kit: savecheck.sh (run after every
  session: SAVES OK after each run tonight), savewatch.sh (logs every write in the save folder to savewatch.log).
- Save watcher: Steam rewrites 1160582305/steam_autocloud.vdf at every game launch (Steam Cloud is on for this game): a likely writer of last night's slot2 change. Watch for slot2 writes at launch/exit.

## 2026-11-02 01:00-01:50 -- feature backlog, horse riding together
- New rule from the user (whole project): work every completable task, flag needs-user items in NOTES/FLAGS.md,
  never ask "continue?". Saved in memory and PLAN.
- Research: docs/research/game-features-web.md (minigames, mounts, systems from wikis), docs/research/
  game-features-troc.md (all 1,175 messages grouped by system), backlog docs/COOP-FEATURES.md (8 areas).
  Key finds: full minigame protocol (JoinMiniGameReq, SpawnMiniGameCharacterReq = the NPC opponent, results,
  leaderboards), dormant MMO party system (OpenPartyReq, InvitePartyMemberReq...), knowledge sharing messages,
  LetterBoxEnterReq/ExitReq (cinematic letterbox), ChangeRideSeatAck (seats; upstream found it a stub on 2976).
- Horse (D30): horse_test.sh rounds horse-on..horse-ride1; mount key is E ("Ride" prompt), dismount R. Status line
  now logs "Local: our horse 0x... at (x, y, z)" (walk-to-mount in tests).
- No cutscene seen in any of 35 captures (only PlayAllPreLoadSequencerAck at loads): cutscene work needs one (F3).
- Shared waypoints (backlog 5.7): new EventKind::Waypoint (14). Our CreateOrChangeSelfDestinationReq (2663: f32 x, y, z |
  u8 flag; y = 0 from the map) is sent 3x (lossy channel); the receiver shows it as a second pin (id kCoopPinId+1,
  next colour). Verified in echo (wp2): set from the map (Esc -> Map tab -> right-click), pin created at the same spot.
  Flag 0 taken as "removed" (not seen yet). Map controls seen: right-click = marker, "Remove Destination", R = centre on Kliff.
- Weather: SyncWeatherStateReq (2689) C>S, 24 B = 2 x u32 0 + 4 floats: the weather is simulated client-side; syncing it
  needs the guest's weather state in memory (later). Time: SetFieldTime not seen in these captures.
- Ping (7.3): Ctrl+F3 sends our position as a Waypoint event with flag 2; the receiver gets the pin + toast "<name> pinged
  their position (see your map)". Verified in echo (ping1). keys.ps1 now knows F1-F5, F11, R, Q, U, H.
- Mounted combat (1.2): horse-combat round: the copy rides beside us and mirrors the mounted swings; no bandit was in
  reach, so damage from horseback stays unverified.
- TELEPORT (big): DoTeleportAck (3052: u32 actor | vec3 | 8 B) moves our player standing or walking, but only up to ~50 m
  per message (30 m and 51 m took; 80, 104 (valid ground), 150, 400 m and 5 km ignored). Hops of 40 m every 0.4 s work
  (tp-hop: 104 m in 3 hops, exact). Fixes the checkpoint revive (far-hopU: dock -> checkpoint, "at the checkpoint") and
  gives Ctrl+F4 "go to your friend". Dev: [dev] kill_key=1 also enables Ctrl+Shift+F4 (tp_offset_m / tp_to "x,y,z").
- SAVE WRITER FOUND: the game AUTOSAVES server-side into slot2 when entering some areas (go-hall4: entering Hernand Town
  at 03:05:15 -> SaveToFileCurrentPlayerAck 2966 "01 00 10 A0 | 02 00 00 00 | 01 01 ..." with no request before it).
  That was the 2026-11-01 21:22 write too. slot2 restored from the 16:04 backup (overwritten copy kept). savecheck.sh
  --fix now restores automatically; save_guard also watches SaveToFileCurrentPlayerAck (logs the slot; warns in co-op).
- Waypoint flag 0 = removal, confirmed live (right-click on an existing destination: "removed (0, 0)").
- Adaptive hop teleport (D31): hops at our current height, retried at +-3/8/15/25 and +40/60/90 m when they do not
  take (targets in the air or in the ground are refused). go-hall4: dock -> Hernand Town plateau (-10699, 635, -3731)
  in ~13 s, over a cliff. Hernand Town: guards + carriage; quest "Unwelcomed Guests: check the commotion in front of
  the castle gate" (marker ~6 m) -> a story scene to capture for F3.

## 2026-11-02 10:50-12:20 -- save editing, dragon test save, flying mount relayed
- User (this session): finish flying mounts, cutscenes and minigames myself; making/altering TEST saves is allowed.
- SAVE FORMAT (docs/research/save-format.md): save.save = "SAVE" header + ChaCha20(LZ4 block(blob)), HMAC-SHA256 of the
  LZ4 block; fixed key (from NattKh's GPL CrimsonSaveEditor). tools/save/cdsave.py (pure Python decode/encode, round
  trip verified), parc.py (schema/TOC), elements.py (list records), patch_save.py (in-place), splice.py (insert/remove
  with self-pointer/tail/TOC repair), make_dragon_save.py. Running the downloaded editor was blocked (external code):
  everything is our own code, written from reading theirs.
- Mounts are MercenarySaveData records (characterKey, mercenaryNo, ownedCharacterKey, ..., item list). Dragon =
  Riding_Dragon_1 (1000799). Needed: Knowledge_CallDragon (1000175) + 4 quests (1000354/1000212/1000305/1000319, state
  5 = completed; 2 = active, 1 = locked), owner = Kliff (1) for the F1 wheel, and NO items: horse gear on the dragon
  crashed the game (loop over 0xD0 equip entries with slot index -1, exe+0x26021B5 / 0x4AFB694). Test save
  build/test-saves/slot103-dragon (Demian's horse record -> dragon, gear removed via splice); testsave.ps1 -Variant.
- Summon: F1 (companion wheel; select with the ABSOLUTE cursor, wheel.ps1 CURSOR:760,740) -> FrameEventCallMercenaryReq
  (hash 0x47279650, category 3, spawn point ~40 m above us) -> UpdateCallMercenaryCoolTimeAck + ActorLoadDataAck of the
  dragon (mount slot 3). H tap = hash 0x54F1C6A6 category 4 (horse); H in the air = hash 0x95DA161E category 4. The
  dragon as MAIN mount: the client sends nothing on H. The dragon flies to a point ~50 m away, hovers 20-30 m up, then
  climbs to ~+65 m; Kliff never grapples on (guide: "Kliff automatically grapples onto it"; likely needs the grapple
  ability / Axiom Force skill the early test save lacks). RideOnVehicleReq (2566: u32, u32, u8) gets no reply.
- CO-OP RESULT (ride4, echo): the mod's capture sees the dragon as "our horse" (0xB0100003), sends it, and the copy
  side loads a dragon copy in the air ("Puppet: horse prepared at (-10365, 578, -3862) (10232 bytes)"); screenshots
  show two dragons flying. So a flying mount's summon/flight reaches the partner with the existing horse path.
  Not verified: a rider on the dragon (ours or the copy's) -- needs a save where Kliff can grapple on (flag F2).
- Dev tool: [dev] script=<file> + Ctrl+Shift+F6 sends c2s/s2c packets from a text file (net/dev_script.*; {P} {U} {POS}).

## 2026-11-02 12:20-13:20 -- minigame capture, cutscene flag, long teleports
- ForcePlayStageDevReq (3056, u32 stage key) is ignored by the retail server (4 stages tried, nothing happened).
- Stage keys (NattKh stage_names.json): minigames are stages (Her_MiniGame_PubArmwrest 16005100 ...).
- Long teleport (travel_test.sh, TO="x,y,z"): 2.2 km in 50 s then a cliff (fix D34), then the sea (no teleports while
  swimming). Mission objective positions come from the save (MissionStateData _uiPosition): "Unwelcomed Guests"
  step 0 "Head to Hernand" (-10685, 633, -3738); arriving by teleport did not advance it (no mission messages).
- NPC finder: build/port3479/npcs.py (ActorLoadDataAck + character_names.json + Transform marker position).
  Hernand pub cellar: NHM_GameHost_1 at (-10662, 617, -3832) runs arm wrestling ("[R] Try arm wrestling" at the
  table; hold R, then hold E "Participate (20)"; intro skippable with Esc; then hold/mash E "Push"). Duo table players
  (NHM_Seotda_Player*) beside it. At game time ~10:50 the table had no opponent and no prompt (NPC schedules).
- ARM WRESTLING CAPTURE (3 matches): InteractionPreemptionStepReq/DoStepReq (player, host 0xB0100373, interaction
  0x000F431C) -> ActivateMiniGameReq (u16 0x4240 | u32 player) -> SetAvoidByCutSceneAck pairs (stage instance
  0x204) during the intro -> JoinMiniGameReq (04 | id | player) + SpawnMiniGameCharacterReq (04 | u64 0x204 | u16 id |
  u32 opponent character 53220 / 54609 | u32 0x20000006) -> SpawnMiniGameCharacterAck (opponent actor) + the fee
  (RemoveItemToInventoryAck) -> JoinMiniGameReq for the opponent + RecoveryActorForMiniGameReq -> (match simulated in
  the client) -> LeaveMiniGameReq (04 | opponent | ... | u32 stage 16005100) -> MiniGameResultReq (u16 id | 00 |
  opponent | 01 00 00 00 | 0 | 1 or 2) -> UnspawnMiniGameCharacterReq. The result bytes were the same for a win
  and a loss. No LetterBox*Req and no skip message: cutscene playback and skip are client-only.
- CUTSCENE FLAG: snapshot diff of .data (uisnap.py idle/cut/post): exe+0x70D2E2F = 1 only while a cutscene plays
  (minigame intro and its exit fade); 0 in free roam, the pause menu, Tab, and the minigame countdown/play. The other
  candidate (exe+0x71C78A3 bit 0) flips by itself. cutflag.py reads both.
- Built: net/coop_activity.* (D32, D33), dev_script "tp x y z", hop tries (D34). The game autosaved into slot2 again
  at 13:02:55 (entering the pub); savecheck --fix restored it (copy kept). Not yet verified live: the activity
  events (the arm wrestling table was empty when retried).
- INCIDENT 13:10-13:55: savecheck --fix restored slot2 with plain `cp` (new file time), so the title's "Continue"
  loaded the USER's slot2 instead of the test save in the act2/act3 runs; the game then autosaved into slot2 (13:33)
  and slot0 (13:52). Both restored from the 2026-11-01 16:04 backup (overwritten copies kept in
  SaveBackups\overwritten-by-test-2026-11-02_13-54 / _13-55); SAVES OK. Nothing was saved by me. Fix: --fix now
  touches the test save after a restore so it stays the newest (restored files keep the current time, so Steam Cloud
  prefers them over any autosaved copy it uploaded). Told the user (FLAGS F5).

## 2026-11-02 13:55-14:30 -- activity verified, knowledge/weather probes, package a7810297
- act_test.sh (fixed: the travel output pipe never closed with the echo peer): from a clean load of the test save
  the activity module saw a cutscene during the load-in, the [dev] skip_test pressed Esc 3 s in ("skipping ours
  too"), and the cutscene ended: detection + injected skip verified. The arm wrestling table had no seated opponent
  in the later runs (10:35-10:55 game time; turned a full circle with R): the minigame notices are not seen live;
  their triggers match the captured layouts byte for byte.
- LearnKnowledgeByCheatReq (2986: player | key | u8 | u8) gets no reply: cheats are off in retail (like
  ForcePlayStageDevReq). Knowledge cannot be granted through the server.
- Weather: SyncWeatherStateReq floats (e.g. 23.15, 0.400, 0.0087, 2.75) are not stored as such in the heap (one hit,
  in a packet buffer): the client computes them when reporting. Matching weather needs the weather system's own
  state (not found).
- Dev script: "poke <hex addr> <hex bytes>" (writable pages only) for memory experiments.
- Package 0.1.0-alpha+a7810297 built, verify-port ALL PASS, Defender clean, installed (DLL hash matches, ini kept).

## 2026-11-02 14:50-16:40 -- user round 3: skip together, versus arm wrestling, time sync, wyvern
- User: minigames vs friends (most popular first), wyvern/griffin flying test, time + weather MUST match, boss fights
  together incl. their cutscenes, and cutscene skip = everyone must press skip (D32 revised).
- SKIP: coop_activity keyboard hook (WH_KEYBOARD_LL thread) holds the player's Esc while the other player is in a
  cutscene within 100 m; both pressed -> each mod presses Esc (marked dwExtraInfo 0xC0C0A7). Verified 4x with [dev]
  skip_test (simulated partner presses 6 s in): "we pressed skip; waiting" -> "both pressed; skipping" -> ended.
  Bug found + fixed: send() inside g_mutex -> std::system_error "resource deadlock" killed the network worker; and the
  activity tick sat inside the player-snapshot block (no snapshot in a minigame). Hook stops blocking if the tick
  stalls (500 ms heartbeat).
- ARM WRESTLING table: game time ~10:27-10:58 only (fresh load = 10:25; dev "time 10:30" reopens it). The prompt
  check by grep of the packet LOG failed because the text log is buffered: detect via cdmp.log Activity lines.
  pub.sh (load + stand at the table facing it, 1500 px turn) then hold R ("Try arm wrestling"), hold E (Participate).
- GAUGE: hardware write breakpoint (dev "watch") on the UI gauge (+0x1B8 of the live UIGamePlayControlCommon_
  MiniGameGauge@uiCommonScript) -> writer exe+0xE3235D (percent = int/100) <- caller exe+0x101D3D0, which reads float
  [[exe+0x71B6CA8] + 0x90] + 0xD30 via locked getter exe+0xA25630. Live: 50 at the start, falls ~15/s idle; poked 100
  = match ends (win), 0 = loss ("Haha! You can't possibly be scared!"), 90 held = match goes on. (MiniGameData +0xE0/
  +0xF4 are animation blend weights, not the gauge.)
- VERSUS (D35): both in a match -> host gauge = 50 + 2 x (host E presses - guest E presses), written every tick; guest
  writes 100 - it. [dev] versus_test=N simulates the partner: idle -> 50..0, "they won"; mashing -> 96, game ended
  (win). Banner "Arm wrestling vs <name>: mash E! You 62 - 38". Result toast from the last gauge value.
- TIME: kFieldTimeClient was stale since 3446 (0x6EB2AD8 = garbage): real exe+0x6EA4998 (copy +0x20). The game clock
  read fails on 3479 -> time sync now uses the capture's estimate (capture_game_clock_ms). [coop] time_sync default 3
  = auto (host sends, guest follows). dev "time 10:30" -> "day 27 10:37 -> day 28 10:30" (read + set verified).
- WEATHER: SyncWeatherStateReq floats not stored as such; trace_op=2689 stack = generic send pipeline (exe+0xAB3xxx);
  ServerWeatherManager = a 0x50-byte helper. Client classes WeatherSimulatorScriptComponent etc. (other namespace).
- WYVERN: Riding_Wyvern_1000 character key = 1004233 (live: string object <- record, key at +8; Bear_1000 = 1003911
  checks the layout). As main mount (Kliff's horse record) H sends nothing; on Demian's record it is not on the F1
  wheel. Guides: adult wyvern needs full growth + a saddle (removed with the gear). Not solved yet.

## 2026-11-02 17:10-17:40 -- weather sync (D37)
- Weather C/D test (18:00 on two days) dropped: the packet logs show weather is not a function of time. The server
  prepares Weather_* stages (PrepareSequencerAck 2380, key at payload+19) at load and later; 9 loads of the test save
  gave 9 different sets (e.g. Sunny+Windy90+Speed07+Cloudy65 / Windy180+Speed04+Cloudy80+Rainy_Hard). SyncWeather-
  StateReq (2689) is only the client's report of the result. Boss stages have their own *_Boss_Weather stages.
- Stage lifecycle (weather, 103-byte prepare): UpdateStageStateAck + AddStageRandomRateAck (by stage key) ->
  PrepareSequencerAck (u8 4|player|player|u64 seq|u16 1|key|0|hash|key|pos...) + StageBranchStateAck (u8 4|player|
  seq|u32 branch|u32 prev|0x22B6EC91|0|0xDAD73925|u32 counter; initial branch 0 prev -1) -> client
  UpdateStageChartEventReq (seq|branch|idx|u8) repeated until UpdateStageChartEventAck (u8 4|player|+req) -> every
  ~60 s client StageBranchStateReq (seq|branch|u16) -> StageOnBranchEventAck (u8 4|player|seq|branch|0) +
  StageBranchStateAck (branch 1, prev, counter+1); the server ends a stage at one of those ticks (DeleteSequencerAck).
- First injection test (dev script s2c, correct 103 bytes): the client ran the stage but spammed ChartEventReq (no
  Ack) and nothing showed. Second (module, 105-byte template typo): client ignored it. Third: weather_sync with the
  stand-in acks: branch requests answered at +2 and +3 min, rain icon and rain after ~1 min (shots rain3-1-1 vs -12).
- tools/port/pkbin.py reads cdmp_packets_*.bin (whole packets; the .log truncates the hex).
- Boss idea from this: boss fights and their cinematics are stages too (cd_boss_*, *_bossencounter_*, Imp_Banquet_Boss_
  Battle_* are prepared near Hernand with pos 0,0,0). The same relay could start the host's boss stage in the guest's
  client (mirroring the host's branch acks instead of standing in). Next.

## 2026-11-02 17:40-17:50 -- stage cinema relay (D38)
- Boss research (upstream pendiente-jefes.md): a boss fight = .pastage + .paseq run by the player's own client; the
  .paseq spawns Boss_00 and holds the cinematics; conditions only look at the local Player. Stage prepares near
  Hernand include Imp_Banquet_Boss_Battle_GreyWolf_Yann/Nairah, _WildHorse (pos 0,0,0, quest-bound). A prepare's tail
  binds stage nodes to world actors: (u32 0x200000nn | u32 actor key | 05 00 00) x n; node 1/2 = the player.
- No boss location data here (no PAZ extractor; all_world_locations.json has none near), so no one-PC boss test.
- cine.sh (pub table, [dev] cinema_test=1): "our cutscene is in stage 16005100 (sequencer 0x1FE)" -> copy 0x7D000000
  (180 bytes) 3 s in -> ended 1 s after ours; the client sent no chart/trigger traffic for the copy (duplicate of a
  running stage). Game fine, match finished. Pub entry autosaved slot2 again; restored (SAVES OK).

## 2026-11-02 17:50-18:10 -- wyvern names, verify-port, versus gating
- Live string scan (strscan.py, 417 hits, build/port3479/wyv_strings.txt): Item_Egg_Wyvern (+ _item_use_0),
  Animal_Baby_Wyvern_1, Riding_Wyvern_1000, Riding_Wyvern_Common, Actor_Vehicle_Wyvern, CallVehicleWyvern_Owner,
  KeepRidingWyvern_Owner, Player_Ride_Wyvern, Knowledge_Wyvern (= 1001881, knowledge_keys_all.json), PetArmor_Fabric_
  Helm_WyvernBaby, Wyvern's Cradle (Knowledge_HiddenPororin_Cradle 1000722). So: egg item -> baby wyvern (pet) ->
  grows into Riding_Wyvern_1000. No griffin mount name in memory (only "Griffin" text).
- Mount calls seen: FrameEventCallMercenaryReq 2643 = u64 mercenary no | u32 kind (3 = H main mount, 2 = F1 wheel) |
  FF | f32 pos | u32 n | 01 -> CallVehicleMercenaryAck 2720. Test saves slot103-wyvern-k(-main) = wyvern records +
  Knowledge_Wyvern in place of the Haunted Hill discovery (make_dragon_save.py now takes the knowledge key).
- verify-port checks kFieldTimeClient at the real 0x6EA4998: ALL OFFLINE CHECKS PASSED.
- Versus now only for arm/hand wrestling stages (stage key of the match from SpawnMiniGameCharacterReq's stage
  sequencer via cinema_stage_key); banner names the game.
- Wyvern keys (ptr_multi.py, record = name ptr | key at +8): Item_Egg_Wyvern 1004389, Animal_Baby_Wyvern_1 1004045,
  Riding_Wyvern_1000 1004233 (again), Knowledge_Wyvern 1001881, CallVehicleWyvern_Owner 1000010 (other record type).
- wyvern-k-main (wyvern as Kliff's main mount + Knowledge_Wyvern): H sends nothing; an injected
  FrameEventCallMercenaryReq for that record (kind 3, as H does for the horse) gets NO answer from the server. So the
  server validates the call against the character (the grown wyvern comes another way: the egg).
- slot103-wyvern-egg (tools/save/make_egg_save.py): inventory slot 7 (22007 x9) -> 1 x Item_Egg_Wyvern, + Knowledge_
  Wyvern. Next: use it from the inventory (UI), see what hatches; growth = days? (dev "day N" can skip them).
- EGG (slot103-wyvern-egg): inventory shows "Wyvern Egg, Keepsake: ... Keep it safe and warm in a nest, and it won't
  be long before the beast inside breaks free." Space = Use -> UseItemReq 3060 (38 B: player | u64 item no 0xFA | u32
  item key | u16 slot 7 | ...) -> HideInventoryByUseItemAck; Kliff carries it (Place Ctrl / Throw Q / Drop R / Store F).
  Dropped = a "Wyvern Egg [E] Carry" object. dev "day 1" (27 -> 28): not hatched. Hopped to Pororin Village
  (-10172, 573, -5243 = Reachwood Ruins, Grace Estate): no nest seen. Flagged F7 (how does it hatch?).
- RPS (rps_go.sh: dev tp to the Hernand rock-paper-scissors spot -10611, 609, -3873): "[R] Play Rock, Paper,
  Scissors" with a Boy (two kids, NPC schedule); at 10:35-10:44 the kids played each other (InteractionPreemption/
  DoStepReq between B0100350/351 and B0100658/65E, interaction 0x0F42B1) and our R did nothing; up close only
  "[E] Greet". Planned versus design (not built): hold our pick key (keyboard hook, like the skip) until both players
  picked, then write the partner's pick as the NPC's and release; needs the pick/NPC-pick fields (gaugehunt.py diff
  during rounds) at a time the boy is free.

## 2026-11-02 18:25-20:10 -- wyvern trip, RPS opened
- User: hatch per method.gg; test RPS myself. Guides (method.gg, allthings.how, nerdschalk, mapmaster): egg cave
  Wyvern's Cradle on Three Brothers' Cliff (SW Delesyia) -> working nest SW of the cave; hatch after minutes (real) /
  ~1 h game time, don't leave; the BABY wyvern is a PET only (not mountable yet). A rideable wyvern: shoot a wild one
  down with a bow near Fort Windridge (S Delesyia), mount it while stunned (temporary, not keepable).
- Places (all_world_locations): Fort Windridge = "Del WindCliffFort" (-6649, 502, -5519); Wyvern's Cradle = "Del
  AbandonedWyvernNest" (-6736, 548, -5330), revive point 26 m away (-6727, 533, -5305).
- Getting there failed: hops from Hernand end in the Bay of Abundance (the game ignores teleports while swimming),
  hops stop at cliffs/walls (Bowsprit Cape; Demeniss wall at -9054, -3868) and from the cape's beach; DoTeleportAck only
  moves ~50 m, so no jump over the 440 m of water. A test save with the start moved to the nest (tools/save/
  move_save.py; TransformSaveData position at 0x365C8A) always loads at Trembling Gorge (-9004, 479, -5066): the game
  rejects a start in Delesyia for this early save. Server cheats (reqsizes.py: MovePlayerByCheatReq = vec3 + u32,
  RequestHireMercenaryForCheatReq = u32 + u32, SpawnCharacterCheatReq = 4,4,4,8,1,1) get no answer on retail.
  Walking: two falls killed Kliff (death screen + Q revive fine). The user's own saves are all near Hernand too.
- RPS: the R prompt was greyed and sent nothing for 8 tries over 7 game days. Knowledge_Minigame_RockPaperSci
  (1002618) was missing (arm wrestling's 1002833 present) -> test save slot103-rps (tools/save/knowledge_swap.py);
  still greyed. The stage (Hernand_MiniGame_RockPaperSci, node bindings: the two kids) waits in branch 0; injecting
  the server's "branch 1" (StageOnBranchEventAck + StageBranchStateAck) opens "Rock, Paper, Scissors [E]"; E ->
  StageBranchStateReq -> server branch 3 + ActivateMiniGameReq 0x4247 -> picks U scissors / F paper / E rock; rounds
  are stage branches: in branch 4 the client draws the boy's hand and sends StageBranchStateReq with a transition
  (1..9) for the combination; the server answers with the outcome branch (texts "Rock loses to paper. I lose." ...).
  rps_open.py writes the injection for the current stage id.

## 2026-11-02 20:10-22:10 -- RPS transitions, challenges (D39, D40)
- RPS edges collected (rps_collect.py, rps_matches.sh; fresh loads reset the child's "once in a while" refusal, a
  skipped game day does not): S-S 1, S-P 2, P-R 3, P-S 4, R-S 7, R-P 8; 9 = "I better get going" (leave). Forced
  rewrites: 6 -> no answer; 5 (client's P-P draw) -> 3 -> no answer (client then re-sent its own). The child's
  actions don't encode the hand; the server checks the outcome against its own random roll -> D39 duel instead.
- User (mid-task): challenges must be a real option, finished by the next stop -> D40 built and tested (duel_test.sh,
  duel_in.sh, pub.sh + pubvs.sh with challenge_test). Found: Ctrl+F5 shifted the pub table's interaction focus to a
  shop item ("Steal/Buy"), so challenge keys are Shift+F5/F6. A leftover rps_matches.sh loop (TaskStop left its
  children running) restarted the game during one test; killed by PID.

## 2026-11-02 22:15-2026-11-03 -- overnight list (user away until 22:00 on 11-03)
- User list (memory overnight-list-2026-11-03): native-looking UI, input-method icons, pad binds, duels with an
  out-of-bounds rule, seeing the friend as the opponent, wyvern save, progression research, shared progression,
  more competitive minigames.
- Done before this entry (commits a0bba8d, 4846330): input/pad_input (XInput hook + DualShock/DualSense HID), the
  action table (input/actions: keyboard + pad chord per action), render/game_ui (Palatino, dialog bands, key caps /
  Xbox / PlayStation glyphs, death screen replica), panel hidden by default, welcome notice; progression research
  (docs/research/progression.md). Cheat routes for progression are dead ends (no answer on retail).
- Duel (D41): coop_duel.*, challenge picker, title cards, ring, opponent bar. Runs (echo peer, [dev] duel_test,
  challenge_test=1): duelpvp1 - the echo bounced our own invite back as an incoming one (fixed: ignored in test mode);
  duelpvp2 - picker, accept, countdown, "Fight!", ring posts and opponent bar all on screen; no swing counted (the
  test character's attacks 0x25DC7FB9 / 0x626B7BEC are in no damage table) and bandits nearby hit us (178/300 HP)
  -> swings from attack-button presses (D41 revision), quiet test save (rps, Hernand), duel clock fixed (the
  caller's now_ms is steady_clock, duel_hud used GetTickCount).
- RPS medallions over both characters (hands at the reveal, "?" / seal while choosing) - built, untested.
- Party journal (D42): party_quests.* + mission_names.inc (tools/gen/mission_names_inc.py). FLAGS F8.
- Nameplate and guest boss bar redrawn game style (ui::nameplate, ui::boss_bar; the boss bar left the F8 panel).
- Dragon: early save lacks rope skills and Blackstar's knowledge -> test save dragon-rope (4 knowledge swaps);
  guides: summoning in mid-air mounts; the manual grapple + Aerial Maneuver costs 200 stamina (test save has 120).
