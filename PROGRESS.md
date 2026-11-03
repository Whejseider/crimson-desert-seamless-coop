# PROGRESS (ROADMAP.md work log)

Started 2026-11-03 (user handed over ROADMAP.md). Older history: NOTES/LOG.md, NOTES/DECISIONS.md.
One-PC testing uses the echo peer ("stand-in"): build/port3479/*.sh; after every test session savecheck.sh --fix,
test save Out, shipped cdmp.ini restored.

## T01 -- Two-PC playtest pack
Plan: list every 2-PC item (joining incl. room code and invite-only, guest loot, shared enemy set, boss cutscene,
final blow, boss death wait, challenges, ending); rewrite docs/tests/PLAYTEST-1.md as numbered tables with
expected result, Pass / Fail boxes and Notes, plus a session record header.
Done: PLAYTEST-1.md rewritten (blocks 0-7, 60 numbered steps). Later tasks append their 2-PC steps there.

## T02 -- Anti-cheat and terms check
Plan: research the game's DRM / anti-cheat / online services; list the in-process vs online pieces; add a start-up
guard (anti-cheat modules in the process, anti-cheat files in the game folder) that keeps every part of Co-op+ off;
dev switch to simulate a hit; README + findings note.
Done: no anti-cheat (Denuvo Anti-Tamper = DRM); server is in-process; online pieces = Steam, Denuvo licence,
cross-save (PA account), sentry/kafka. src/mod/online_guard.* ; guard tripped (dev online_guard_test=winmm.dll):
log 8 lines, nothing installed, message box shown; normal start: "co-op allowed", all hooks as before.
docs/research/online-integrity.md, README "IS IT SAFE?". Note: when tripped the overlay is not installed either, so
the message is a plain Windows box (an anti-cheat could flag the DXGI hook).

## T03 -- Version handshake
Plan: Hello / Welcome carry the game's exe version (protocol 10 -> 11; the reader rejects trailing bytes, so a bump);
ClientSession refuses another mod build or game version on both sides (no Connected, so no spawn / events), with a
message naming both versions and what to update; shown as a game-style notice; [dev] fake_peer_build to test it.
Done: unit tests (Welcome with another build / game refused, host refuses a Hello and still answers with its own
versions, matching Hello connects). In game: ver1/ver2 (fake build 12345678): "Can't play together: ... has Co-op+
build 12345678 on game ?, you have build 2526DEB6 on game 1.0.0.3479. Both install the same Co-op+ zip." as a notice,
no puppet; ver3 (control): connects, puppet spawns. Older (protocol 10) builds are refused by the old protocol check.
Note: the relay (cdmp_peer) announces build 0 / no game version: not compared (dev tool only).
