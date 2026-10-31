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
| `patches/0002-coopplus-steam-join.patch` | Co-op+ M1: Steam "Join Game", overlay invites (Ctrl+F1), leave (Ctrl+F2), rich presence, `[net] role=auto` (host your world, become a guest on a Steam join). New files `src/mod/net/steam_join.*`, `join_args.h` (unit-tested); `mod.cpp` runs sessions back to back (`run_coop`) and re-applies role settings (`apply_role_settings`); `room_guest_resolve_lobby()`. Regenerate with `git diff -- upstream/cd-coop-3446` after changing these files. |
| `patches/0003-utf16-ini-profiles-no-crlf.patch` | Upstream bug: `.gitattributes` has `kit/** text eol=crlf`, which also converts the UTF-16 `cdmp.ini.*` profiles; on a Windows checkout every LF gains a single 0x0D byte and the UTF-16 text turns to garbage (seen 2026-10-31). Exempts `kit/**/cdmp.ini*`. Worth sending upstream. |
| `patches/0001-enemy-zone-ctor-gcc-clang.patch` | `EnemyZoneMachine(Settings s = {})` needs the nested struct's default member initializers before the class is complete; MSVC accepts it, GCC/Clang don't. Split into two constructors so the upstream unit tests also build on Linux. No behaviour change. |
