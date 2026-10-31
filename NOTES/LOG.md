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
