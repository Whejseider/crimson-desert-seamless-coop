# Crimson Desert Seamless Co-op+ -- source of this build

GPL-3.0 (LICENSE). Built on Crimson Desert Multiplayer (cdmp) by ByJunter and cd-coop-3446 by nouzert1 (CREDITS.md).

- `upstream/cd-coop-3446/` -- the mod (cdmp.asi): the upstream code with the Co-op+ changes.
  Build: Visual Studio 2022 (C++ workload) + CMake, then `upstream/cd-coop-3446/scripts/build.ps1`.
- `src/sim/`, `third_party/tomlplusplus/` -- the party-scaling and settings layer it links (MIT toml++).
- `config/scaling.toml` -- the scaling curves shipped next to the mod.
- `kit/coopplus/` -- the installer, uninstaller, vanilla switch, diagnose script and README.
- `tools/package-coopplus.ps1` -- makes the player zip (it downloads Ultimate ASI Loader v9.7.4 by ThirteenAG, MIT,
  through `tools/fetch-asi-loader.ps1` and checks its hash).
