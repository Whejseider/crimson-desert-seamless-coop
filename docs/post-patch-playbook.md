# Post-patch playbook (roadmap T38)

What to do when Steam updates Crimson Desert. Co-op+ is made for one exact exe; on any other it stays off by itself
("Co-op+ is off: this mod was made for game version ...") and the game runs normally. Nothing breaks for players;
this is how to bring the mod to the new build. Two ports were done this way: 3446 -> 3460 and 3460 -> 3479
(docs/research/port-3460.md, port-3479.md: what moved and how each value was found).

## 0. Keep the old exe
Copy the old `CrimsonDesert.exe` to `build/exe-archive/<version>/` BEFORE updating (git-ignored). Diffing two builds is
far easier than finding things from scratch.

## 1. What changed (10 minutes)
- `CrimsonDesert.exe` file version, PE timestamp, SizeOfImage (the mod's log prints all three at start:
  "Host version: ... PE timestamp ... image size").
- `upstream/cd-coop-3446/scripts/verify-port.ps1 -Exe "<path to the new exe>"`: checks every recorded code head and
  address offline. Each FAILED line is one value to re-find.

## 1b. Fastest: map with both exes (3527 port, 2026-11-04)
With the old exe archived, `build/port3527/portmap.py` maps everything at once: `map_all.py` (code by masked
instructions, globals and vtables through the code that addresses them; prints hits / votes), then `gen_offsets.py`,
`retarget.py`, `retarget_verify.py` (edit the old -> new tables). Check every line with more than one hit: template
copies (the action layers) need mapping through their caller, the nearest hit was wrong. docs/research/port-3527.md.

## 2. Re-find what moved (build/port3479 tools; copy them to build/port<new>)
| What | Tool | How |
|---|---|---|
| Code with a recorded head (hooks) | `heads.py`, `near.py` | search the head bytes in the new image, take the hit nearest the old address; most moved by the same small shift per region |
| Vtables (RTTI) | `vt.py` | class name -> type descriptor -> locator -> vtable |
| Data globals | `globals.py` | the upstream signatures (one match each), RIP-relative target decoded |
| Message numbers (Troc table) | `troc.py` (live, title screen) + `tools/port/check_opcodes.py <troc_dump>` | the game's own name -> opcode table; the mod also resolves names at start ("Sniffer: 1175 message types resolved") |
| Message layouts | `reqsizes.py <ReqName>` | the field sizes each request's handler reads, in order |
| Wire layout (MoveData, StartAction) | captures + `tools/port/pkbin.py` | net/wire_layout.h documents the 3460 change; compare a capture of walking / attacking |
| Anything else | `port_scan.py`, `tools/port/pkbin.py` | scans and capture decoding |

Write the new values into `src/mod/game/offsets_<build>.h` (one file per build, the old one kept), point
`offsets.h` at it, and update the version check (`game_build_supported`, the "made for game version" text).

## 3. Verify (1-2 hours)
1. `verify-port.ps1` again: ALL PASS.
2. Build (`scripts/build.ps1`): unit tests pass.
3. One-PC checks with the echo peer (build/port3479/*.sh, copied and renamed):
   - `peer_test.sh` -- the other player's copy loads, is drawn and follows (log "the other player is in our world",
     "made visible");
   - `bandit_test.sh`, `zone3`-style run with `dev.zone_copy_test=1` -- enemy copies load, no crash;
   - `rejoin_test.sh` -- 5 kill-and-rejoin cycles clean;
   - `audit_test.sh` -- three sessions, state audit clean;
   - `revive_test.sh`, `seat_late.sh`, `duel_pvp.sh`, `hunt_kill2.sh` -- revive, arm wrestling seat, duel, hunt;
   - `stress_test.sh` -- no crash under a bad network.
4. The log must have none of: "not recognised", "hook failed", "crash guards", "Crash log". The crash guards retry
   five times at start (T06); a failure there is logged with the safetyhook error.
5. `build/port3479/savecheck.sh --fix` after every session; test save Out; shipped cdmp.ini back.

## 4. Ship
`tools/package-coopplus.ps1`, Defender scan of the extracted zip, install with `kit/coopplus/install.ps1 -NoPause`,
CHANGELOG entry ("Game <version>: every address re-found ..."), push.

## Lessons from the two ports
- Code moved by small per-region shifts (+0, -0x30, -0x50, -0x70); vtables and the Troc table by +0x1000; data by
  ~+0xFB0. Message numbers did not change between 3460 and 3479 but did between 3424 and 3460.
- 3460 reordered the MoveData block (+4 bytes): positions read 4 bytes early make every enemy fail the zone test
  silently (fixed: enemy_load::find_t0 checks the layout). Check a capture, not only the heads.
- A head that matches but whose rip-relative operand moved (the minimap icon site) must be re-recorded.
