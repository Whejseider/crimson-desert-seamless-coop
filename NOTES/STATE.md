# STATE (single source of truth for resuming)

**Phase A (setup + research), 2026-10-31.** Phase B waits for the user's exact words: "GO: game testing is clear."

## Done
- Project skeleton, CMake, C++20. 42 unit tests pass (Linux). Windows DLL `CrimsonDesertCoop.asi` cross-builds (MinGW);
  CI workflow builds it with MSVC. Release script makes a zip.
- DLL (M0-ready, untested in game): fail-safe startup thread, exe-name filter, vanilla mode, logging, loaded-module
  report (loaders/ReShade), Steam detection via the game's steam_api64 flat API, data-driven read-only probe
  (`config/probe.toml`).
- Pure logic: scaling curves (+ stagger, QoL compensation, radius), loot modes, progression merge + guest-save policy,
  TOML config with friendly errors, versioned protocol, snapshot interpolation, signature scanner.
- Research notes: NOTES/RESEARCH/*.md; claims register; architecture + milestones (docs/architecture.md).

## In progress
- Engine/DRM/loader research agent (anticheat-drm.md, loaders-and-dll-mods.md, pearl-abyss-statements.md).

## Open questions for the user (pivotal)
1. Build on the GPL-3 cdmp / cd-coop-3446 code (fast, project becomes GPL-3) vs clean-room (MIT, slower). See
   NOTES/RESEARCH/cdmp-codebase.md. Recommendation: build on it.
2. Phase B needs the game PC: this session is a cloud container. Options: attach the PC via Remote Control, or run
   Claude Code on the PC. Also needs a second PC + Steam account (a friend) for two-player tests.
3. Network policy blocks nexusmods.com and reddit.com from this session (Nexus permissions are therefore UNVERIFIED).

## Next steps
1. Fold the engine/DRM research into CLAIMS C8 and docs/research.md.
2. Ask the user the questions above; record answers in DECISIONS.md.
3. If "build on cdmp": vendor cd-coop-3446 as `upstream/` (GPL-3), map its modules onto our layers, replace hardcoded
   addresses with signatures, switch our license to GPL-3, add CREDITS.md.
4. Prepare the Phase B M0 package (loader + asi + configs) and the two-PC test kit.

## Waiting on GO
- Everything in docs/tests/M0.md.
