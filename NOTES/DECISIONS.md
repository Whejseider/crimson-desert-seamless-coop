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
