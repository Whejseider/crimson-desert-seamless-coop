# STATE (single source of truth for resuming)

**Phase A done; local PC session (session 2) started 2026-10-31. Waiting for the user's exact words "GO: game testing is clear."**

## Where the project lives now
- Local folder on the user's PC: `C:\Users\asher\CrimsonDesert-SeamlessCoop` (D15). Short path on purpose: the upstream
  sources break MSVC under deep folders (260-char MAX_PATH; we don't change the system long-path setting).
- Work branch: `phase-a/local-pc-setup` (pushed). `main` = Phase A from session 1.

## Done
- Skeleton, CMake C++20; 42 own unit tests + upstream's test suite pass on Linux; MinGW builds; CI workflow.
- Our layers: sim/, net/, game/, compat/, core/. Commented TOML configs.
- Research: NOTES/RESEARCH/*.md, docs/research.md, CLAIMS.md.
- D10 (user): build on cdmp via nouzert1/cd-coop-3446 -> `upstream/` (GPL-3.0), CREDITS.md, docs/integration-plan.md.
- **Session 2 (on the PC):**
  - Game exe confirmed read-only: `D:\SteamLibrary\steamapps\common\Crimson Desert\bin64\CrimsonDesert.exe`,
    FileVersion **1.0.0.3446** (matches upstream `offsets_3446.h`), 332,786,072 bytes, written 2026-10-31 02:14.
  - Toolchain: VS Build Tools 2022 17.14 (MSVC 14.44.35207), bundled CMake 3.31 + Ninja, Windows SDK 10.0.26100.
    Nothing to install. VsDevCmd needs `...\Microsoft Visual Studio\Installer` on PATH (for vswhere).
  - MSVC build of our targets compiled cleanly up to the upstream test sources, which failed only on path length
    (long scratch path). Re-run from the short folder.
  - **Nexus permissions verified first-hand** for all 27 listed mods (docs/mod-permissions.md). Nothing changes the
    plan: all Comfort Pack features stay "reimplement the idea + credit". Permissive: Fat Stacks, Infinite Stamina,
    PrivateStorageAnywherePLUS, JMM, Barber 2.04. CDAutoLoot is hidden (dropped). Ultimate ASI Loader is MIT, v10.0.2.

## User preferences (2026-10-31)
- Contact the user only at pivotal moments, real roadblocks, or direction-changing decisions. Don't ask what the brief
  or notes already answer.
- Chrome (Claude in Chrome) is signed in to the user's Nexus and GitHub for our use; use our own tab only.
- Goal: play with friends in an official-feeling co-op mod.

## Blockers / waiting on the user
1. **GO** for game testing (another session may be testing Elden Ring on this PC).
2. Two-player tests need a second PC + Steam account that owns the game (a friend). Denuvo: max 5 PCs/day.

## Next steps
1. In `C:\Users\asher\CrimsonDesert-SeamlessCoop`: MSVC build of ours (`cmake -G Ninja`, Release) + ctest; MSVC build
   of upstream `cdmp.asi` (upstream/cd-coop-3446, its own CMake) + its tests. Record results in LOG.
2. Pre-GO prep that needs no game: read upstream `offsets_3446.h` against our sigscan design; plan integration step 2
   (one DLL: upstream + cdcoop_core) per docs/integration-plan.md and start it.
3. After GO: M0 per docs/tests/M0.md + upstream's single-PC relay test (`cdmp_peer --mode relay`).
4. Then signatures, then M1 Steam lobbies (integration-plan.md).
5. Courtesy note to nouzert1 (GitHub issue) offering patches back; user to approve before posting.
