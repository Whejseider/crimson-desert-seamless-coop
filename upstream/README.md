# upstream/

Copies of the GPL-3.0 code Co-op+ builds on, kept as close to pristine as possible so upstream fixes can still be
merged (`tools/sync_upstream.sh`). Our own changes live in `src/`. A change to upstream code is a small patch in
`patches/`, listed below and worth offering upstream.

| Folder | Project | Author(s) | Commit | Licence |
|---|---|---|---|---|
| `cd-coop-3446/` | https://github.com/nouzert1/cd-coop-3446 (port of cdmp to game build 1.0.0.3446) | ByJunter (original cdmp, https://github.com/ByJunter/CrimsonDesertMultiplayer), nouzert1 ("CD Coop Port") | b7ae133 (2026-10-31) | GPL-3.0 |

Left out of the copy: `third_party/ultimate-asi-loader/v9.7.4/dinput8.dll` (a binary). Ultimate ASI Loader (MIT, by
ThirteenAG) is downloaded from its official GitHub release when packaging.

## Local patches (re-applied by tools/sync_upstream.sh)
| Patch | Why |
|---|---|
| `patches/0001-enemy-zone-ctor-gcc-clang.patch` | `EnemyZoneMachine(Settings s = {})` needs the nested struct's default member initializers before the class is complete; MSVC accepts it, GCC/Clang don't. Split into two constructors so the upstream unit tests also build on Linux. No behaviour change. |
