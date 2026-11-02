# First two-player playtest: checklist (D2)

One evening, about 90 minutes. Follow the blocks in order; each block only needs the ones before it.
A = host (you), B = friend. Both on game 1.0.0.3479, both installed from the SAME zip.

## Before you start (both)
- [ ] Install from the zip ("Install Co-op+.cmd"); it backs up your saves.
- [ ] In bin64\cdmp.ini, under `[sniffer]`, set `record=1` (packet recording, for bug reports; 50-100 MB).
- [ ] Steam overlay on. You are Steam friends.
- [ ] Rule for the whole evening: **nobody saves while the other player is connected.** Quit without saving.
- [ ] Before each block, both press **Ctrl+F12** at the same moment (marks the logs so they line up).

## 1. Joining (10 min)  -- details: docs/tests/M1.md
- [ ] Both load a save. A presses Ctrl+F1 and invites B; B accepts. Both panels (F8) say connected.
- [ ] B appears in A's world and A in B's: walking, running, jumping. Name tags, minimap arrow, map pin.
- [ ] Toasts: "<B> joined your world" on A, "You are in <A>'s world" on B.
- [ ] B leaves with Ctrl+F2, then joins again with "Join Game" from A's entry in the Steam friends list.

## 2. Looks and movement (10 min)  -- details: docs/tests/M2-M3.md "M2"
- [ ] Glide, climb, swim, ledge pull-up, attack, ride: note anything that freezes or snaps.
- [ ] B changes helmet or weapon: A sees it within a couple of seconds (and the other way round).

## 2b. Riding, waypoints, pings, teleport (10 min)
- [ ] Both call a horse (H) and ride (E; R dismounts): each sees the other ride their own horse beside them.
- [ ] Ride 300 m together: no snapping or vanishing; dismount together.
- [ ] A sets a waypoint on the map (right-click): B sees a second pin there.
- [ ] A presses Ctrl+F3: B gets "<A> pinged their position" and a pin.
- [ ] B walks 200 m away, then presses Ctrl+F4: B lands next to A. Try 800 m too and note what happens.

## 3. Fighting together (20 min)
Go somewhere with ordinary enemies near A (a bandit camp).
- [ ] B sees A's enemies; B's own enemies there are gone (and come back when B walks 250 m away).
- [ ] B hits an enemy: its HP drops on A's screen too; B can finish it.
- [ ] Enemies attack either player (usually whoever hit them last); their blows hurt B for real.
- [ ] **No friendly fire (A11):** A and B swing at each other for a while: nobody loses HP.
- [ ] Loot (A4): after a shared kill, B loots the body: B gets its own drop; A loots the same body: A gets its own.
- [ ] Chests: both open "the same" chest: both get the contents (per-player loot, D24).

## 4. Deaths (15 min)  -- A12
On one PC the game's own death screen never appears once the other player's character has been loaded;
Co-op+ then shows its own "Death has found you  [Q] Instant Revive  [U] Revive at Checkpoint" box (centre of
the screen, about 4 s after the death). Note which one each of you gets: if the game's own screen DOES appear
with two real players, that is good news (the one-PC test clones your own character, which may be the cause).
- [ ] B dies to enemies while A fights on: B gets a death screen (the game's or Co-op+'s); A is not affected and
      keeps fighting. On A's screen B's character falls.
- [ ] B presses Q (Instant Revive): B gets up in place with part of their health; on A's screen B gets up too.
- [ ] B dies FAR from the last checkpoint (walk a few hundred metres first) and presses U: with the game's own
      screen B goes to the checkpoint (loading screen). With Co-op+'s box B gets full health and is moved to the
      checkpoint in a few quick hops. If moved: watch for falling through the
      ground; on A's screen B's character should jump there too.
- [ ] Same with A dying while B fights on.
- [ ] A fall death (jump off a cliff): same death screen, same revive.
- [ ] Each of you: is the number of instant revives (shown under "Instant Revive" in the game's own screen)
      used up as usual? With Co-op+'s box, if Q does nothing (none left?), use U and note it.

## 5. Bosses (15 min, only if a boss is handy)
- [ ] B helps; A lands the final blow; the boss's cinematic plays normally on A.
- [ ] Both logs: "Boss: enemy 0x... treated as a boss" for the boss (it is recognised by its health pool).
- [ ] A9: B dies during the fight: B sees "Your partner is still fighting the boss. Hold on..." (no Q/U). A wins:
      B gets up where they fell with full health within a few seconds (log: "the other player beat the boss").
      This step was never tested (needs two players).
- [ ] Both go down: each sees "Everyone is down. [U] Revive at Checkpoint"; both press U.

## 6. Ending (5 min)
- [ ] B leaves (Ctrl+F2): A's world is normal, no leftover copies; B's own enemies are back.
- [ ] Both quit WITHOUT saving.

## Afterwards: what to send
From each PC: `bin64\cdmp.log`, the newest `bin64\cdmp_packets_*.log/.bin` pair, and a few lines on what went
wrong at which Ctrl+F12 mark. "Diagnose Co-op+.cmd" puts a report on the Desktop that includes the log.
Then set `record=0` again.
