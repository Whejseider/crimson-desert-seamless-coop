# PROGRESS (ROADMAP.md work log)

Started 2026-11-03 (user handed over ROADMAP.md). Older history: NOTES/LOG.md, NOTES/DECISIONS.md.
One-PC testing uses the echo peer ("stand-in"): build/port3479/*.sh; after every test session savecheck.sh --fix,
test save Out, shipped cdmp.ini restored.

## T01 -- Two-PC playtest pack
Plan: list every 2-PC item (joining incl. room code and invite-only, guest loot, shared enemy set, boss cutscene,
final blow, boss death wait, challenges, ending); rewrite docs/tests/PLAYTEST-1.md as numbered tables with
expected result, Pass / Fail boxes and Notes, plus a session record header.
Done: PLAYTEST-1.md rewritten (blocks 0-7, 60 numbered steps). Later tasks append their 2-PC steps there.
