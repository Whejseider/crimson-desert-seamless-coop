# First two-player playtest: checklist

About two hours. Follow the blocks in order; each block only needs the ones before it.
**A = host, B = friend.** Both on game 1.0.0.3479, both installed from the SAME zip.

How to record: for each step tick **Pass** or **Fail** and write anything odd in **Notes** (what you saw, roughly
when, which Ctrl+F12 mark). A step that could not be tried: leave both boxes empty and write why.

| Session record | |
|---|---|
| Date | |
| Mod build (F8 panel title / zip name) | |
| A's PC / B's PC (GPU, Windows) | |
| Connection (same house / internet, rough ping from F8) | |

---

## 0. Before you start (both)

| # | Do | Expected | Pass | Fail | Notes |
|---|---|---|---|---|---|
| 0.1 | Install from the zip ("Install Co-op+.cmd"). | "Installed." and "Saves backed up: N file(s)". | [ ] | [ ] | |
| 0.2 | In `bin64\cdmp.ini`, under `[sniffer]`, set `record=1`. | (packet recording for bug reports, 50-100 MB) | [ ] | [ ] | |
| 0.3 | Steam overlay on; you are Steam friends. | Shift+Tab opens the overlay in game. | [ ] | [ ] | |
| 0.4 | Rule for the whole session: **nobody saves while the other player is connected.** Quit without saving. | | | | |
| 0.5 | Before each block both press **Ctrl+F12** at the same moment. | The log gets "=== MARK n ===" (lines the two logs up). | [ ] | [ ] | |

## 1. Joining and leaving

| # | Do | Expected | Pass | Fail | Notes |
|---|---|---|---|---|---|
| 1.1 | Both load a save. A presses Ctrl+F1 and invites B in the Steam overlay; B accepts. | Both F8 panels say connected; A: "<B> joined your world"; B: "You are in <A>'s world". | [ ] | [ ] | |
| 1.2 | Look at each other. | Each sees the other's character (walking, running, jumping), name tag fading with distance, minimap arrow, map pin. | [ ] | [ ] | |
| 1.3 | B presses Ctrl+F2. | B is back in their own world; A: "<B> left". No leftover character of B on A's screen. | [ ] | [ ] | |
| 1.4 | B right-clicks A in the Steam friends list > "Join Game". | B joins again as in 1.1. | [ ] | [ ] | |
| 1.5 | B quits the game completely; A sends a new invite (Ctrl+F1); B accepts from the Steam overlay / client. | Steam starts B's game; after B loads a save B lands in A's world. | [ ] | [ ] | |
| 1.6 | Room code: B leaves (Ctrl+F2). B types A's 7-letter room code (A's F8 panel) into B's `cdmp.ini` under `[room] code=`, restarts the game, loads a save. | B joins A's world. Afterwards B empties `code=` again. | [ ] | [ ] | |
| 1.6b | Crash and rejoin: B ends the game in Task Manager while connected, starts it again, loads, A invites again (or B uses "Join Game"). Repeat 3 times. | A: "<B> left your world" within ~5 s and B's character disappears (no frozen copy); after each rejoin exactly ONE B on A's screen; B's bandit copies near A are not doubled. | [ ] | [ ] | |
| 1.6c | A alt-tabs to the desktop for a minute, comes back. | Still connected; B kept moving on A's screen after A came back. | [ ] | [ ] | |
| 1.6d | Different versions: B installs an older Co-op+ zip (or one of you is on another game patch) and tries to join. | Both see "Can't play together: ... has Co-op+ build X on game ..., you have build Y ..." and what to install; nothing of the other appears. B reinstalls the same zip. | [ ] | [ ] | |
| 1.6e | Drop-in mid-event: B joins (a) while A has the world map open, (b) while A fights bandits, (c) while A watches a cutscene, (d) while A is on a loading screen (fast travel), (e) during a boss fight. | Each time exactly one B appears on A's screen once A is back in the world; nothing stuck (A's menu closes normally, the cutscene ends normally, the boss fight continues). | [ ] | [ ] | |
| 1.6f | Resync: B presses Shift+F8 (pad: hold View + RB) near A in a bandit camp. | B: "Resynchronising with <A>..."; within a few seconds A is back on B's screen once and the bandits are there once (not doubled); A sees B left and came back. | [ ] | [ ] | |
| 1.7 | Invite-only: A sets `join=invite` under `[net]`, restarts. B tries "Join Game" without an invite. | B cannot join; with an invite (Ctrl+F1) B can. A sets `join=friends` back. | [ ] | [ ] | |

## 2. Looks and movement

| # | Do | Expected | Pass | Fail | Notes |
|---|---|---|---|---|---|
| 2.1 | B glides, climbs a wall, swims, pulls up a ledge, attacks the air. | A sees each move without freezing or snapping. | [ ] | [ ] | |
| 2.2 | B changes helmet, then weapon. Then A does. | The other sees the change within a couple of seconds. | [ ] | [ ] | |
| 2.3 | Both call a horse (H) and ride (E; R dismounts). Ride 300 m together. | Each sees the other ride their own horse beside them; no snapping or vanishing. | [ ] | [ ] | |
| 2.4 | (If one has Blackstar) summon the dragon from the F1 wheel and fly a little. | The other sees the dragon flying with the rider. | [ ] | [ ] | |
| 2.5 | A sets a map waypoint (right-click). | B sees a second pin there. | [ ] | [ ] | |
| 2.6 | A presses Ctrl+F3. | B: "<A> pinged their position" and a pin. | [ ] | [ ] | |
| 2.7 | B walks 200 m away, presses Ctrl+F4. Then 800 m. | B lands next to A (short hops, no loading screen); note what happens at 800 m. | [ ] | [ ] | |
| 2.8 | Time and weather: compare the clock under the minimap and the sky. | B shows A's time of day and A's weather (rain on both or neither). | [ ] | [ ] | |

## 3. Fighting together (a bandit camp near A)

| # | Do | Expected | Pass | Fail | Notes |
|---|---|---|---|---|---|
| 3.1 | Both walk up to the camp; both count the bandits you can see from the same spot. | Same number in the same places on both screens (B sees A's set). | [ ] | [ ] | |
| 3.2 | B walks 250 m away from the camp, then back. | B's own bandits come back while away and are hidden again (replaced by A's) when back. | [ ] | [ ] | |
| 3.3 | B hits a bandit. | Its health drops on A's screen too; B can finish it; it dies on both screens. | [ ] | [ ] | |
| 3.4 | Let the bandits attack. | They attack either player (usually whoever hit them last); their blows hurt B for real. | [ ] | [ ] | |
| 3.5 | Party scaling: A notes how many hits a bandit takes with B connected and alone (B leaves for a moment). | Noticeably tougher with B in (about x1.5 health; the bar dips and partly refills after each blow). | [ ] | [ ] | |
| 3.5b | Menus mid-fight: while both fight bandits, A opens the map for 20 s, then the inventory, then the pause menu. | On B's screen A stands still with "in a menu" under the name; the bandits turn to B and leave A alone; A takes no damage while in a menu; B's game never pauses or stutters; after A closes the menu both carry on. | [ ] | [ ] | |
| 3.6 | **No friendly fire:** A and B swing at each other outside a duel. | Nobody loses health. | [ ] | [ ] | |
| 3.7 | **Loot for the guest:** after a kill B helped with, B loots the body. | B gets its own drop. A loots the same body: A gets its own. Note what each got. | [ ] | [ ] | |
| 3.8 | Kill one bandit only B hits (A does not touch it). B loots it. | B gets a drop; note whether B got experience / skill points as alone. | [ ] | [ ] | |
| 3.8b | Loot rules: both set `loot_mode=first_come` under `[coop]` in cdmp.ini (restart). Kill a bandit together; A loots it first. | B's body of it shows no loot (empty) right after A looted. Next bandit: B loots first -> A's body of it is empty. Set `loot_mode=per_player` back. | [ ] | [ ] | |
| 3.8c | Crime: B steals from a stall or hits a townsperson in sight of a guard (B's own world). | B becomes wanted in B's world; A gets "<B> is now wanted by the guards (in their own world)"; A's world: no wanted state, guards ignore A. | [ ] | [ ] | |
| 3.9 | Chests: both open "the same" chest. | Both get the contents (each in their own world). | [ ] | [ ] | |

## 4. Deaths

On one PC the game's own death screen never appears once the other player's character has been loaded; Co-op+ then
shows its own "Death has found you  [Q] Instant Revive  [U] Revive at Checkpoint" box about 4 s after the death.
Note which one each of you gets.

| # | Do | Expected | Pass | Fail | Notes |
|---|---|---|---|---|---|
| 4.1 | B dies to enemies while A fights on. | B gets a death screen (game's or Co-op+'s, note which); A is not affected; on A's screen B falls. | [ ] | [ ] | |
| 4.2 | B presses Q. | B gets up in place with part of their health; on A's screen too. | [ ] | [ ] | |
| 4.3 | B walks a few hundred metres from the last checkpoint, dies, presses U. | B is at the checkpoint with full health (loading screen, or quick hops with Co-op+'s box); no falling through the ground; A sees B move there. | [ ] | [ ] | |
| 4.4 | Same as 4.1-4.3 with A dying while B fights on. | Same results. | [ ] | [ ] | |
| 4.4b | Teammate revive: B goes down near A (not in a boss fight). A walks to B and holds Shift+F7 (pad: hold View + LB) for 3 s. Then again, but A is far away for 30 s. | B: "<A> can revive you (N s)" under the death screen; A: "Hold to revive <B>", then "Reviving <B>... N%". B gets up where they fell with full health; A: "You revived <B>", B: "<A> revived you". After 30 s down the offer ends; Q / U still work. | [ ] | [ ] | |
| 4.5 | A fall death (jump off a cliff). | Same death screen, same revive. | [ ] | [ ] | |
| 4.6 | Instant revives used up as usual? | The count under "Instant Revive" goes down; with none left Q does nothing and U works. | [ ] | [ ] | |

## 5. Bosses (only if a boss is handy; A's story boss)

| # | Do | Expected | Pass | Fail | Notes |
|---|---|---|---|---|---|
| 5.1 | Walk into the boss's intro together. | B sees the same intro cinematic as A (not just a notice). | [ ] | [ ] | |
| 5.2 | During the intro only A presses Esc, then B. | It skips only once both pressed ("waiting for <name>" on A until then). | [ ] | [ ] | |
| 5.3 | Fight; B does most of the damage. | B's hits lower the boss's health on both screens; B's boss health bar shows. Both logs: "Boss: enemy 0x... treated as a boss". | [ ] | [ ] | |
| 5.4 | **Final blow:** let B try to land the last hit. | The boss cannot die from B's hit; A lands the final blow; A's victory cinematic plays normally and A's quest advances. | [ ] | [ ] | |
| 5.5 | **Boss death wait:** B dies during the fight. | B sees "Your partner is still fighting the boss. Hold on..." (no Q/U). | [ ] | [ ] | |
| 5.6 | A then wins. | B gets up where they fell with full health within a few seconds (log: "the other player beat the boss"). | [ ] | [ ] | |
| 5.6b | Per boss (Matthias, Kailok the Hornsplitter, the Reed Devil, Kearush the Slayer; docs/research/boss-audit.md): did the boss ever hang on B (a grab on B's character that never ended)? Did every phase start? Did the end cinematic play for A and the barrier drop? | No hang; phases and end as alone. If a boss hangs on B: set `enemies_target_copy=0` under `[coop]` for both and note the boss. | [ ] | [ ] | |
| 5.7 | Both go down. | Each sees "Everyone is down. [U] Revive at Checkpoint"; both press U and can try again. | [ ] | [ ] | |

## 6. Challenges

| # | Do | Expected | Pass | Fail | Notes |
|---|---|---|---|---|---|
| 6.1 | A presses Shift+F5 (pad: hold View/Touchpad + D-pad Up). | Picker "Challenge <B> to: Duel / Rock, paper, scissors / Hunting contest"; arrows or D-pad Left/Right move the gold choice. | [ ] | [ ] | |
| 6.2 | A picks RPS, Shift+F5 sends; B presses Shift+F5. | B saw "<A> challenges you to rock, paper, scissors". Both press U / F / E (pad X / Y / B): same two hands (also over both characters) and same round winner on both screens; first to 2. | [ ] | [ ] | |
| 6.3 | Duel: a few metres apart, A picks Duel, B accepts. | Gold ring (20 m), "3, 2, 1, Fight!", the opponent's health bar at the top. | [ ] | [ ] | |
| 6.4 | Hit each other. | The one hit flinches and loses health on BOTH screens; at 3% "Defeat" / "Victory" on the right screens; both healed; round 2 starts. | [ ] | [ ] | |
| 6.5 | One runs out of the ring. | Red ring + "Return to the duel ground"; after 10 s that player loses the round. | [ ] | [ ] | |
| 6.6 | Hunting contest: both hunt for 3 minutes. | Each kill: "Hunt / <animal> +N" for the hunter; the score line changes on BOTH screens; same Victory / Defeat at 0:00. Note any kill that did not count. | [ ] | [ ] | |
| 6.7 | Hernand pub (game time ~10:27-10:58; the host's time is used): both at the arm wrestling table; A challenges, B accepts; both hold R then E. | Note whether the table is staffed for both (with the one-PC stand-in it was sometimes empty while connected). | [ ] | [ ] | |
| 6.8 | During the match. | Each sees the OTHER player in the opponent's chair, arms locked (not the game's NPC). | [ ] | [ ] | |
| 6.9 | Mash E. | The faster one wins on BOTH screens; series toast; after the match the NPC is back and the friend stands up. | [ ] | [ ] | |
| 6.10 | Shift+F6 on an invite; Shift+F5 during a challenge. | The invite is declined; the challenge ends with the final series. | [ ] | [ ] | |

## 7. Ending

| # | Do | Expected | Pass | Fail | Notes |
|---|---|---|---|---|---|
| 7.1 | B leaves (Ctrl+F2). | A's world is normal (no leftover copies); B's own enemies are back; B's time and weather are B's own again. | [ ] | [ ] | |
| 7.2 | A quits the game while B is still connected (redo 1.1 first). | B gets "<A> left" and is back in their own world with no leftovers. | [ ] | [ ] | |
| 7.3 | Both quit WITHOUT saving. | | [ ] | [ ] | |

## Afterwards: what to send

From each PC: `bin64\cdmp.log`, the newest `bin64\cdmp_packets_*.log/.bin` pair, and this file with the boxes
ticked. "Diagnose Co-op+.cmd" puts a report on the Desktop that includes the log. Then set `record=0` again.
