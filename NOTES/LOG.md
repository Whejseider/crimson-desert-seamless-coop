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
- 05:40 Test plans: docs/tests/M0.md rewritten for the cd-coop-3446 base; new docs/tests/M1.md (two-PC Steam join).
- Upstream on GitHub: still b7ae133 (our import is current). Their 2026-10-31 two-PC script says the core question
  (does the guest SEE the host's character) is still open; 270e2cc position fix not yet verified live.
- Party scaling (D18): common/party_scaling.h (pure, tests), net/party_scaling.*, hooks in local_capture (VaryHPAck)
  and guest_hits (scale + ledger), [scaling] enabled=0 health_2p=1.7, panel line, 5 s log line. heal_back first
  computed loss*(1-1/k) -> 17 HP gave 6 back with k=1.7f (float); now loss - round(loss/k), matching guest blows.
  Build clean, all unit tests pass, loopback passes, root MSVC build + ctest pass. Our C4701 (protocol.cpp) fixed.
- Self-review of run_coop: join callbacks now registered during the Steam-name wait; a role change always
  reconfigures the guest's enemy zone (a role=guest player who left kept hiding enemies as host). Build/tests/loopback OK.
- 06:00 Nameplates (brief 6.2): common/nameplate.h fade (full to 25 m, 35% at 200 m, hidden from 600 m), text shrinks
  to 80%; F8 now toggles only the panel, Ctrl+F8 the nameplate; [overlay] nameplates=1. Panel moved into draw_panel().
  Occlusion not done (needs the depth buffer; live work). Build + tests OK. Visual check needs GO.
