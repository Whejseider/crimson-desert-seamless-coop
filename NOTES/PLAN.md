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
- [~] A2 Enemy copies move, attack and die in step with the host's enemies (follow at ~300 ms and die together:
      verified; attack animations not yet checked by eye)
- [x] A3 The friend's hits on a copy damage the host's real enemy (attack layout fixed for 3479; verified kill)
- [ ] A4 Loot from shared kills: the friend loots their own roll from the copy. BLOCKED on 3479: the "twin" spawn
      request logs the player out; twins are off. Needs a new, safe way to give the friend their roll
- [ ] A5 The friend's own enemies near the host are hidden (no doubles) and come back when apart (suppress:
      lab test on one PC, then switch the default from "dry run" to real once A1-A3 hold)
- [ ] A6 Bosses: safety rules (host lands the killing blow; boss copies refused) still hold on 3479
- [ ] A7 Turn enemy copies back on by default once A1-A6 pass
- [ ] A8 Boss scaling by player count, for up to 4 players (user request 2026-11-01): boss health/damage grow with
      the party (curve in config/scaling.toml, e.g. x1.0 / 1.7 / 2.3 / 2.8); bosses must stay hard with a full party
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
- [ ] C6 Next-update readiness: one script that re-finds every address after a game patch (port tools), exe kept

## D. Hand-off for the playtest
- [ ] D1 A one-page "How to play with a friend" (install, invite, join, what to expect, how to send logs)
- [ ] D2 A two-PC test checklist (what to try in what order, what to look for)
- [ ] D3 Final package: Defender-clean, installed here, link ready for the friend

## E. Needs the second player (after D)
- [ ] E1 Live two-PC session through Steam, following D2
- [ ] E2 Fix what that session finds, then repeat
