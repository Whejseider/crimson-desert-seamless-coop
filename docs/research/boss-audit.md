# Boss mechanics audit for two players (roadmap T13, 2026-11-03)

A boss fight in Crimson Desert is a scripted **stage** run by the client of the player whose world it is (the host):
the stage owns the phases, the cinematics, the arena barrier and the "battle end" that closes the fight and moves the
quest on (net/boss_safe.h). The guest fights a **copy** of the boss (the host's enemy relayed), so every mechanic
below is judged by one question: can the second player break the host's script or get stuck themselves?

Sources: the mod's own boss research (net/boss_safe.h, boss_rules.h, docs in upstream/), the boss guides
([PowerPyx boss guide](https://www.powerpyx.com/crimson-desert-boss-guide-all-bosses),
[8bittoast boss list](https://8bittoast.com/crimson-desert-bosses/),
[dropreference DE guide](https://dropreference.com/de/blog/guide/crimson-desert-boss-guide-alle-strategien-reihenfolge-schwaechen-2026)).

## Mechanics that assume one player, and what Co-op+ does

| Mechanic | Assumes one player because | Risk with two | Co-op+ handling | Status |
|---|---|---|---|---|
| Killing blow / battle end | The stage waits for its own death of the boss to play the end cinematic and `set_battleEnd` | A guest's cheat kill skips the script: no end, barrier stays, host quest stuck | Boss safety (`[coop] boss_safe=1`): the guest's blows still do damage but never land the killing blow; the host always does | Built, unit-tested (boss_rules); 2-PC: PLAYTEST 5.4 |
| Finisher on a staggered ("fatal") boss | The fatal state waits for the player's finishing move | A guest finisher (`DieByRunOutFatalReq`) skips the sequence | Not sent on bosses (boss_safe) | Built |
| Stagger / groggy from blows | The stage times phases on its own staggers | A relayed stagger breaks a phase | The guest never staggers a boss (boss_safe) | Built |
| Phase triggers (health thresholds, `BossPhase`) | Fired by the host's client when the boss's health crosses them | None: the guest's damage lands as real damage in the host's world | Damage relayed; phases fire on the host natively | Built; 2-PC: 5.3 |
| Entrance / phase / end cinematics | Played by the host's stage | Guest sees nothing, or both skip separately | The host's stage cinematic is replayed in the guest's client (`[coop] stage_cinema`); nothing lands while it plays; skip only when both pressed | Built (D32, D38); 2-PC: 5.1, 5.2 |
| Arena barrier / lock | A stage object in the host's world | The guest's world has no barrier: the guest can walk out | Harmless (no softlock: the guest can always walk back in or use Ctrl+F4); the host's barrier works as normal | By design |
| Boss targeting | The boss's AI picks among players / allies | The boss may pick the guest's copy | The copy is a valid target like a player (`enemies_target_copy`); its blows hurt the guest for real; while the guest is in a menu it is not a target (T09) | Built; 2-PC: 5.3 |
| Grabs / paired attacks (boss holds the player) | A paired animation with the victim | The copy cannot answer a pair animation: the boss could wait on it | Fallback: `[coop] enemies_target_copy=0` makes every enemy (bosses included) leave the copy alone if a boss is seen to hang on the guest | Fallback available; watch in 2-PC (5.3) |
| Player death in the arena | One death = retry | One player down while the other fights | Boss death wait (`[coop] boss_death_wait=1`): the downed player waits; partner wins = up where they fell; both down = both revive at the checkpoint | Built (D29); 2-PC: 5.5-5.7 |
| Difficulty | Tuned for one | Two players halve the time | Party scaling for bosses x1.7 health (`scaling.toml`) | Built (D28) |
| Loot / rewards | The host's world drops | The guest has no boss of its own | The host loots; the guest's own world has no boss to loot (story bosses are per world); `loot_mode` covers ordinary enemies only | By design, documented |
| Quest credit | The host's quest | The guest is not on the host's story step | Only the host's quest moves; the guest gets the party journal notice (D42) | By design |

## The first five bosses (for the two-player check)
Order per the guides: Myurdin (end of the combat tutorial; usually before co-op is possible), Matthias (two health
bars), Kailok the Hornsplitter (one bar), the Reed Devil (three bars, two sections), Kearush the Slayer (health
draining fog causing Confusion). For each, PLAYTEST-1 block 5 is run once with the guest helping; anything that
hangs is noted with the boss's name and the Ctrl+F12 mark.

Kearush's fog is an area effect of the host's stage: the guest's world has no fog (no damage, no Confusion for the
guest). Accepted (a softer fight for the guest beats a broken one); noted for the playtest.
