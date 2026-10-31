# Decisions

## 2026-10-31 D1: Project location (superseded by D13)
- Options: (a) new standalone repo, (b) `CrimsonDesert-SeamlessCoop/` folder inside `ashersheets-jpg/elden-force-source` on branch `claude/seamless-coop-mod-t4lqdd`.
- Chosen: (b) for now. This session runs in a cloud container whose GitHub access is scoped to that repo and branch. Nothing in the Elden Force project is touched (only a new top-level folder and a new, path-filtered workflow `.github/workflows/cdcoop.yml`).
- Revisit: moving it to its own repo (history can be split with `git subtree split`) before a public release. Suggested to the user.

## 2026-10-31 D2: Toolchain
- Dev box (Linux cloud): CMake 3.28 + Ninja, GCC 13 for native unit tests, MinGW-w64 13 (posix threads) for the Windows DLL cross-build.
- Release/CI: MSVC on GitHub `windows-latest` (static CRT, so no VC++ redistributable is needed). Same sources build on both.
- Why: the brief asks for MSVC/CMake; no Windows machine is available in this session, so MinGW proves the DLL compiles and CI makes the MSVC build.

## 2026-10-31 D3: Steam access without the SDK at build time
- Options: link `steam_api64.lib` from the Steamworks SDK vs. resolve the flat C API from the game's own `steam_api64.dll` at runtime (GetProcAddress).
- Chosen: runtime resolution. No SDK files in the repo, no version lock to the SDK the game shipped with, and if an export is missing co-op stays off instead of the game failing to start. We never call `SteamAPI_Init` (the game owns it).

## 2026-10-31 D4: Loader
- Ship the mod as `CrimsonDesertCoop.asi` for Ultimate ASI Loader (ThirteenAG), which the Nexus survey found is how existing CD DLL mods load (ASI in `bin64`, loader as winmm/dinput8/version.dll). We don't ship a proxy DLL of our own, so we coexist with other ASI mods. To be verified in Phase B (M0).

## 2026-10-31 D5: Config format and library
- TOML, parsed with toml++ v3.4.0 (MIT, vendored single header in `third_party/`), exceptions off. Bad values log an error and keep the default; a config typo never stops the game.

## 2026-10-31 D6: Data-driven M0 probe
- The probe reads signatures and pointer chains from `config/probe.toml`, so offsets found in Phase B can be tried without rebuilding. Reads use ReadProcessMemory on our own process, so a wrong pointer logs `<unreadable>` instead of crashing.

## 2026-10-31 D7: Default scaling curve
- Boss health 1.0/1.7/2.3/2.8 (the brief's starting curve). Regular enemies slightly gentler (1.0/1.5/2.0/2.4), elites in between. Damage scales mildly (up to x1.3-1.4). All in `config/scaling.toml`, to be tuned in play tests.

## 2026-10-31 D8: Guest saves
- Default `guest_save_mode = "separate_slot"`: guests play a co-op copy of their character and their single-player save is never written. Backups before every session are on by default. (Brief 6.3.)

## 2026-10-31 D9: Comfort Pack = our own implementation of the ideas
- Other authors' data patches are mostly "ask first" (unverified) and would have to match exactly on both PCs. We reimplement the ideas as host-synced values in our DLL, credit the mods that inspired them, and list compatible mods as companions. See `docs/mod-permissions.md`.

## 2026-10-31 D10: Build on cdmp / cd-coop-3446 (user decision)
- Options: build on the GPL-3 code (fast; project becomes GPL-3) / clean-room MIT (slow) / contact authors first.
- User chose: build on it. Imported nouzert1/cd-coop-3446 at b7ae133 unchanged into `upstream/` (minus a bundled
  binary), project LICENSE = GPL-3.0, CREDITS.md added. Upstream kept near-pristine (patches listed in
  `upstream/README.md`) so we can keep merging its fixes; it was updated today.
- Plan: `docs/integration-plan.md`.

## 2026-10-31 D11: Phase B runs on the user's PC attached to this session (user decision)
- The user will attach their gaming PC through Remote Control. No folders were shared yet at the time of asking.
- Still gated by "GO: game testing is clear" for launching the game / screen control. Shell and file work on the PC are
  allowed before GO (brief section 0 permits files and shell), but nothing is installed into the game folder before GO.

## 2026-10-31 D12: Upstream portability patch
- `EnemyZoneMachine(Settings s = {})` fails on GCC/Clang. Split into two constructors
  (`upstream/patches/0001-...`) so upstream's unit tests run on Linux and in CI. No behaviour change.

## 2026-10-31 D13: Own repository, fully separate from Elden Force (user instruction)
- The user wants Co-op+ entirely separate from the Elden Force repo and from any Elden Ring work.
- Moved the project to its own repository `crimson-desert-seamless-coop` with its history (git subtree split of the
  four Co-op+ commits; no Elden Force commits or files in it). CI workflow moved to `.github/workflows/build.yml`.
- The Co-op+ commits were never pushed to elden-force-source (push was refused), and the local Elden Force branch was
  reset to its original commit, so the Elden Force repo is untouched on GitHub and locally.
- The integration cannot create repositories (403), so the user creates the empty repo; then we push `main`.

## 2026-10-31 D14: Remote Control attach is not possible (Windows PC) -> proposed: local Claude Code session on the PC
- list_computer_folders: computer DESKTOP-SM0R9J3 offers one folder, `JumpRing` (the Elden Ring project; never to be
  used for Co-op+), under "cannot_be_used": cloud sessions can only attach folders on Mac/Linux computers; this PC runs
  Windows. So D11 (attach the PC to this cloud session) cannot work.
- Proposed to the user: run Phase B in a Claude Code session on the PC itself (desktop app, local), in its own folder
  cloned from https://github.com/ashersheets-jpg/crimson-desert-seamless-coop. That session has the game (D:), Chrome,
  MSVC/screen control (after GO); it starts from README_FOR_CLAUDE.md -> NOTES/STATE.md.

## 2026-10-31 D15: Local project folder on the PC
- The user asked Claude to choose the folder. Options: (a) keep the app's per-session folder (deleted with the session,
  path ~200 chars), (b) `C:\Users\asher\CrimsonDesert-SeamlessCoop`, the brief's suggested name, beside (not inside)
  the JumpRing/Elden Ring project.
- Chosen: (b). It survives the session, and the short path keeps upstream sources under Windows MAX_PATH for MSVC
  (attempt 2 failed with C1083 on a 260+ char path). We do not change the system long-path setting.

## 2026-10-31 D16: Nexus permissions verified; Comfort Pack approach unchanged
- All "ask first" mods stay idea-only (reimplement + credit); nothing is bundled. Permissive mods (Fat Stacks, Infinite
  Stamina, PrivateStorageAnywherePLUS, JMM, Barber 2.04) could be adapted with credit, but the co-op rule (D-policy 7:
  gameplay patches must match on both machines) still favours our own host-synced config.
- Live Transmog and CDUMM: GitHub says MIT, Nexus says ask first. We follow the stricter Nexus terms until the authors
  clarify; no code reuse for now.

## 2026-10-31 D17: M1 join flow uses Steam's own UI, not an in-game menu (for now)
- Options: (a) ImGui co-op menu with mouse/keyboard (needs a WndProc hook and input blocking, cannot be tested before
  GO), (b) Steam overlay for invites + "Join Game" + rich presence, with two hotkeys and a panel hint.
- Chosen: (b). Steam's overlay already is the friends picker players know; it needs no input hook; everything but the
  live flow is testable offline. The ImGui menu can come later (M2/M7) for settings.
- Mechanics: callbacks via SteamAPI_RegisterCallback (the exe pumps RunCallbacks), launch argument +connect_lobby,
  lobby id validated as a Chat-type CSteamID. A join ends the session and starts a guest session (CoopLink destructor
  restores hidden enemies and removes the puppet), then role settings are re-applied.
- Two players only (upstream's limit); lobbies stay friends-only, 2 members.

## 2026-10-31 D18: Party scaling as "damage taken / k", host side, experimental and off by default
- Options: (a) raise the enemies' max HP in memory (needs new RE of the HP pool and of boss phase thresholds; only
  possible live), (b) scale the damage enemies take: the guest's blows are divided by k before guest_hits applies them,
  the host's native blows get (1 - 1/k) given back through VaryStatCheatReq right after the VaryHPAck. Same result as
  health x k, built only from mechanisms upstream already uses live.
- Chosen: (b). A ledger of our own injected HP losses keeps the guest's blows from being scaled twice; kills are never
  undone; only hostile enemies in combat (enemy_relay) get anything back; the guest side never scales (host-auth).
- Risk: a positive VaryStatCheatReq delta (a heal) has never been sent in a live test. So `[scaling] enabled=0` by
  default, including in the package, until the M3 test shows it works (HP bar dips then recovers a little).
- Two players only for now: one value `health_2p` in cdmp.ini; config/scaling.toml takes over with integration step 2.

## 2026-10-31 D19: Save guard = automatic backup + warning, not blocking saves
- The sniffer is passive by design (it can observe and inject, not drop), and dropping SaveAutoReq could leave the
  game's save timers waiting. So: copy %LOCALAPPDATA%\Pearl Abyss\CD\save (5.4 MB here) to
  Documents\CrimsonDesert-CoopPlus-SaveBackups\auto-<time>-before-coop when a co-op session connects (at most every
  10 min, newest 10 kept), and log + show a panel notice when SaveAutoReq / SaveToFileCurrentPlayerReq goes out while
  connected. On by default ([coop] save_guard=1): it only copies files.

## 2026-10-31 D20: Integration step 2, first slice: our sim layer inside cdmp.asi for scaling curves
- Upstream's CMake builds our src/sim (+ toml++) into the mod when the tree sits inside Co-op+ (CDMP_COOPPLUS_SIM).
- The mod reads bin64\cdcoop\config\scaling.toml (our format, unit-tested loader); 2-player health for regular enemies
  and for bosses (boss_is_boss) via sim::compute (clamps, QoL compensation). cdmp.ini [scaling] enabled stays the
  master switch; the TOML can only turn scaling off. Other TOML files (loot, progression, comfort) wait for the
  features that use them.

## 2026-10-31 D21: [net] join = friends (default) | invite
- role=auto makes every Co-op+ player a host with a friends-only lobby and "Join Game"; upstream's host also accepts
  any friend who is playing. That is the convenient default (and upstream's behaviour), kept as `join=friends`.
- `join=invite`: the lobby is created private (k_ELobbyTypePrivate, joinable only through an invite), rich presence
  has no "connect" (no Join Game button), and the host locks only onto a Hello from a member of its lobby
  (GetNumLobbyMembers / GetLobbyMemberByIndex, exports checked). Room codes do not work in this mode (by design).
