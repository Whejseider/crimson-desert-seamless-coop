# STATE (single source of truth for resuming)

**Phase A (setup + research) complete, 2026-10-31. Waiting for the user's exact words "GO: game testing is clear."**

## Done
- Skeleton, CMake C++20; 42 own unit tests + upstream's test suite pass on Linux; Windows builds (MinGW) of our
  `CrimsonDesertCoop.asi` and both test exes; CI workflow (MSVC builds of ours and of upstream cdmp.asi).
- Our layers: sim/ (scaling, loot, progression, config), net/ (protocol, interpolation, Steam flat-API probe),
  game/ (sigscan, data-driven probe), compat/, core/ (fail-safe start, vanilla mode, logging). Commented TOML configs.
- Research: NOTES/RESEARCH/*.md (engine, DRM, loaders, PA statements, co-op prototype/news, other co-op mods, Nexus
  survey, cdmp codebase), docs/research.md summary, CLAIMS.md, mod-permissions.md.
- **D10 (user): build on cdmp via nouzert1/cd-coop-3446** -> imported to `upstream/` (GPL-3.0), LICENSE now GPL-3.0,
  CREDITS.md, docs/integration-plan.md.
- **D11 (user): Phase B on the user's PC attached via Remote Control.**

## Facts from the user
- Crimson Desert is installed through Steam on the **D: drive** (likely `D:\SteamLibrary\steamapps\common\Crimson Desert\bin64\CrimsonDesert.exe`, the same default cdmp's deploy script uses; confirm the exact path once the PC is attached). Before GO: read-only access only (file version, folder listing, hashing the exe for signature work); nothing installed, game not launched.

## Blockers / waiting on the user
1. **GitHub push refused (403)**: the Claude GitHub App has no write access to ashersheets-jpg/elden-force-source.
   Commits are local in the cloud container only (lost if it's reclaimed). User: reconnect GitHub / install the app.
   Once the PC is attached, push from there as a fallback.
2. **PC not attached yet**: list_computer_folders returned nothing. User: Claude desktop app > Settings > Claude Code >
   Remote Control, add a folder (e.g. a new `CrimsonDesert-SeamlessCoop` dev folder, NOT the Elden Ring project).
3. **GO** for game testing.
4. Two-player tests need a second PC + Steam account that owns the game (a friend). Denuvo: max 5 PCs/day activation.
5. Network policy here blocks nexusmods.com/reddit.com: Nexus permissions UNVERIFIED (can check from the PC later).

## Next steps
1. When the PC is attached (before GO): copy/clone the project there, push the branch from there if GitHub is still
   refused here, check for MSVC/VS Build Tools + CMake, build upstream `cdmp.asi` and ours with MSVC. Verify Nexus
   permissions with curl/browser from the PC.
2. After GO: M0 per docs/tests/M0.md + upstream's single-PC relay test (`cdmp_peer --mode relay`). Record the exe
   build, which `offsets_3446.h` entries still hold (game is at patch 2.04.00 + hotfix).
3. Integration step 2 (one DLL: upstream + cdcoop_core), then signatures, then M1 Steam lobbies (integration-plan.md).
4. Courtesy note to nouzert1 (GitHub issue) offering patches back; user to approve before posting.
