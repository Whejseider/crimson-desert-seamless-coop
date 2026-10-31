# Crimson Desert Seamless Co-op+

Play Crimson Desert (PC, Steam, base game) with friends in co-op that feels official: join through Steam, shared
story and world progress, loot for everyone, enemies and bosses that scale with the party, names over heads and
friends on the map, and an optional Comfort Pack of quality-of-life tweaks.

**Status: alpha, not yet tested in the game** (waiting for the first in-game session). The co-op engine is cdmp by
ByJunter as ported to game build 1.0.0.3446 by nouzert1 (`upstream/`, see CREDITS.md); Co-op+ adds Steam "Join Game"
and invites, rich presence, Steam names over heads, party scaling (experimental) and a one-click installer.
Licence: GPL-3.0.

## For players
Build the package (below), unzip it, double-click `Install Co-op+.cmd`, start the game through Steam, press
Ctrl+F1 to invite a friend. The package's README.txt has the details and the limitations. Back up your saves
(`%LOCALAPPDATA%\Pearl Abyss\CD\save`; the installer does it for you).

## For developers (Windows, Visual Studio 2022 Build Tools)
- Mod + tools + unit tests: `upstream\cd-coop-3446\scripts\build.ps1`
- Offline checks against the installed game exe: `upstream\cd-coop-3446\scripts\verify-port.ps1 -Exe <...\CrimsonDesert.exe>`
- Two mod instances on one PC, no game: `upstream\cd-coop-3446\scripts\e2e-loopback.ps1`
- Player package: `tools\package-coopplus.ps1` -> `dist\CrimsonDesert-SeamlessCoop-<version>.zip`
- Our own pure layers (sim, protocol, config) and their tests: `cmake -S . -B build\msvc -G Ninja` (inside VsDevCmd)
- Changes to upstream code are kept as patches in `upstream/patches/` (see `upstream/README.md`).
- Test plans: `docs/tests/M0.md` (one PC), `docs/tests/M1.md` (two PCs). Project state: `NOTES/STATE.md`.
- Design: `docs/architecture.md`, `docs/integration-plan.md`, `docs/comfort-pack.md`, `docs/mod-permissions.md`.
