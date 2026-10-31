# Crimson Desert Seamless Co-op+

Play Crimson Desert (PC, Steam, base game) with friends in co-op that feels official: join through Steam, shared
story and world progress, loot for everyone, enemies and bosses that scale with the party, names over heads and
friends on the map, and an optional Comfort Pack of quality-of-life tweaks.

**Status: early development (Phase A, research and foundations). Not playable yet.** Built on cdmp by ByJunter and
its port by nouzert1 (see CREDITS.md). Licence: GPL-3.0.

- Design: `docs/architecture.md`, `docs/integration-plan.md`, `docs/protocol.md`
- Settings (commented): `config/*.toml`
- Build (Linux dev box): `cmake -S . -B build/linux -G Ninja && cmake --build build/linux && ctest --test-dir build/linux`
- Windows DLL: `cmake -S . -B build/win -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/mingw-w64-x86_64.cmake` or MSVC
  (`cmake -S . -B build -A x64`); release zip: `tools/release.sh`
- Always back up your saves (`%LOCALAPPDATA%\Pearl Abyss\CD\save`) before testing.
