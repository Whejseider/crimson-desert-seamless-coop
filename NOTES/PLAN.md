# Plan: ready for the first two-player playtest

Goal: you and a friend can install one zip, join through Steam, and play together in your world. Each of you sees
the other, fights the same enemies, and gets your own loot, without crashes and without touching your saves.

How I work it (the user's standing instructions, 2026-11-01): I go through this list in order, decide routine
things myself (logged in DECISIONS.md), and don't stop to ask. I only ask about changes that would really alter
gameplay. While waiting on any answer, I work on the next item. After each item: commit, push, notes, and the
installed package updated. Status: [x] done, [~] in progress, [ ] to do.

## Already done (single PC, game 1.0.0.3479)
- [x] Port to 3460, then 3479; the mod switches itself off on unknown game versions
- [x] Friend's character loads, is visible, walks, jumps and climbs; name tag + health bar; map pin
- [x] Gear changes shared live (both ways)
- [x] Overlay crash with NVIDIA DLSS Frame Generation fixed; name tags hidden over menus
- [x] Enemy recognition fixed (3460 layout); game-clock fallback fixed
- [x] Loot design confirmed as per-player (Lootr-style), D24

## A. Shared enemies (the big one; all testable on one PC with the test save)
- [x] A1 Enemy copies load without crashing (3 crash paths guarded: CC tick, animation update, late update;
      6 bandit copies through a full fight, no crash)
- [x] A2 Enemy copies move, attack and die in step with the host's enemies (follow at ~300 ms, attack, die together)
- [x] A3 The friend's hits on a copy damage the host's real enemy (attack layout fixed for 3479; verified kill)
- [~] A4 Loot from shared kills: the friend loots their own roll from the copy. New design (the twin spawn logs the
      player out on 3479): the friend's own hidden native of the same kind is killed for the loot. Built; the
      pickup itself needs the two-PC test (one PC cannot hide natives)
- [x] A5 The friend's own enemies near the host are hidden (no doubles) and come back when apart (lab: 25 hidden,
      all restored); real hiding is now the default
- [ ] A6 Bosses: safety rules (host lands the killing blow; boss copies refused) still hold on 3479
- [x] A7 Enemy copies on by default ([coop] enemy_copies)
- [ ] A8 Boss scaling by player count, for up to 4 players (user request 2026-11-01): boss health/damage grow with
      the party (curve in config/scaling.toml, e.g. x1.0 / 1.7 / 2.3 / 2.8); bosses must stay hard with a full party
- [x] A10 Enemies choose between both players (user question 2026-11-01): make the friend's copy a valid target in
      the host's world so the game's own AI picks between the two players (distance, who hits it...), and
      blows on the friend's copy reach the friend as real damage (aggro + player_hits; port the 2976-era aggro
      addresses to 3479, then turn it on). Done: on by default; enemies switch to whoever hits them; their blows
      on the friend's copy arrive as real damage (-29 hp), with reactions and lethal blows on
- [~] A11 No friendly fire (user request 2026-11-01): players never hurt each other. By design already (hits are only
      relayed for copied ENEMIES; a blow on the other player's copy goes nowhere); keep it so when A10 lands and
      check it in the two-PC test. Code checked 2026-11-01: blows are relayed only onto enemy copies and only from
      enemies; the two-PC check is in docs/tests/PLAYTEST-1.md
- [~] A12 Independent deaths (user 2026-11-01): each player dies/respawns normally (game's own respawn point), the other
      is unaffected; the copy follows. Blocker resolved as far as one PC allows (D26): once a copy has been loaded
      the game's death screen never comes, so Co-op+ shows the same Q/U choice. Instant revive verified; checkpoint
      revive gives full health but moves you only sometimes (best effort). Left: the real loading-screen path for
      U, and the two-PC check (the problem may be echo-only)
- [ ] A9 Boss-fight deaths (user request 2026-11-01): a player who dies during a boss fight does not respawn until the
      teammates beat the boss (then everyone respawns/revives) or everyone is down (then the whole party restarts
      the fight)

## B. Riding, movement and looks
- [ ] B1 Horse copy (riding together): test, fix, decide default (currently off)
- [ ] B2 Animation spot-check: combat moves, gliding, swimming, climbing, mounting
- [ ] B3 Party scaling (experimental): sanity check numbers on 3479
- [ ] B4 Time of day / weather sync check (time_sync is off; decide)

## C. Session, safety and polish
- [ ] C1 Steam session: lobby, rich presence, invite dialog, "Join Game" path as far as one PC can test
- [ ] C2 Save guard: backup on session start, warning on saves during co-op, verified on 3479
- [ ] C3 Game error logger (stale since 3446): port it or drop the warning
- [ ] C4 Panel and messages read well (join/leave toasts, errors in plain words)
- [ ] C5 Installer, uninstaller and "Vanilla mode" checked end to end on 3479
- [~] C6 Next-update readiness: one script that re-finds every address after a game patch (port tools), exe kept;
      tools/port/check_opcodes.py checks every hardcoded opcode against the live table (D27)

## D. Hand-off for the playtest
- [ ] D1 A one-page "How to play with a friend" (install, invite, join, what to expect, how to send logs)
- [~] D2 A two-PC test checklist (what to try in what order, what to look for): docs/tests/PLAYTEST-1.md drafted
- [ ] D3 Final package: Defender-clean, installed here, link ready for the friend

## E. Needs the second player (after D)
- [ ] E1 Live two-PC session through Steam, following D2
- [ ] E2 Fix what that session finds, then repeat
