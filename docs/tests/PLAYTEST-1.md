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
| 2.3b | Mount edge cases: B rides 200 m, dismounts, walks 30 m away and calls the horse again (H), rides, dismounts. Then B switches to another horse (if B owns two) and rides it. If a horse dies in a fight, note it. | On A's screen B's horse is always where B's horse really is (never a second horse back where B first called it), B rides the new horse after a switch, a dead horse disappears. | [ ] | [ ] | |
| 2.4 | (If one has Blackstar) summon the dragon from the F1 wheel and fly a little. | The other sees the dragon flying with the rider. | [ ] | [ ] | |
| 2.5 | A sets a map waypoint (right-click). | B sees a second pin there. | [ ] | [ ] | |
| 2.6 | A presses Ctrl+F3. | B: "<A> pinged their position" and a pin. | [ ] | [ ] | |
| 2.7 | B walks 200 m away, presses Ctrl+F4. Then 800 m. | B lands next to A (short hops, no loading screen); note what happens at 800 m. | [ ] | [ ] | |
| 2.7b | Summon and party travel: A walks 150 m away and presses Shift+F4 (pad: View + right stick click). Then A fast travels to an Abyss Nexus. | B: "<A> asks you to come to them: Ctrl+F4 go", Ctrl+F4 lands B beside A (on solid ground, not inside rock). After A's fast travel B: "<A> travelled far away: follow them". A pressing Shift+F4 again within 30 s: "You can call B again in N s". | [ ] | [ ] | |
| 2.7c | Partner audio (T33), headphones on: A stands still; B walks and runs past A at 2 m, then 20 m, then 60 m away; B swings a weapon at the air near A; B rides past. | A hears B's footsteps, swings and horse, louder near and fading with distance (from the right side). Note anything silent (voice lines of B's character are not expected). | [ ] | [ ] | |
| 2.7d | Player colours (T34): look at the other's name tag and the partner frame (left); then F8 > "Player colours" > colour-blind safe; then "My friend's map pin colour" to another number and set a waypoint. | A (host) sees B in blue with a round mark; B (guest) sees A in gold with a diamond; colour-blind safe: sky blue / orange. B's chat lines in B's colour. The pin changes colour (the game's palette). | [ ] | [ ] | |
| 2.7e | Resting together (T35): both at a campfire; B sits on a bench or chair nearby, then A (host) holds Ctrl on the campfire > Wait. Then B tries Wait too. | A sees B sit (and B sees A). After A's Wait, B's clock jumps to A's new time within a few seconds. B's own Wait is undone by the host's time (expected: time follows the host). | [ ] | [ ] | |
| 2.7f | Spoiler guard (T39): B sets F8 > "My friend's story" > hidden. A finishes a quest step B has not reached; later A walks into a boss / story scene with B near. | B: "<A>: Quest Updated / A quest you have not reached yet (name hidden)"; B's game does not start A's story scene (B only sees "<A> is watching a cutscene"). A quest B is also on is still named. | [ ] | [ ] | |
| 2.7g | Group photo (T44): A sends "Hold still: photo!" (Ctrl+F3, last entry); B strikes a pose; A opens the pause menu > Photo Mode and frames both. | B's character stands in A's photo in its pose; no Co-op+ labels (name tag, frame, feed) in the shot. Note if B's copy is missing or T-posed in photo mode. | [ ] | [ ] | |
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
| 3.5c | Anti-grief: both set `guest_finishes=0` under `[coop]` (restart). B hits a bandit until it should die. Then B tries to hit a townsperson of A's world (none is visible to B: only enemies are shared). | The bandit stops at 1 HP on A's screen until A hits it (A gets the kill); B cannot affect A's townsfolk. Set `guest_finishes=1` back. | [ ] | [ ] | |
| 3.6 | **No friendly fire:** A and B swing at each other outside a duel. | Nobody loses health. | [ ] | [ ] | |
| 3.7 | **Loot for the guest:** after a kill B helped with, B loots the body. | B gets its own drop. A loots the same body: A gets its own. Note what each got. | [ ] | [ ] | |
| 3.8 | Kill one bandit only B hits (A does not touch it). B loots it. | B gets a drop; note whether B got experience / skill points as alone. | [ ] | [ ] | |
| 3.8b | Loot rules: both set `loot_mode=first_come` under `[coop]` in cdmp.ini (restart). Kill a bandit together; A loots it first. | B's body of it shows no loot (empty) right after A looted. Next bandit: B loots first -> A's body of it is empty. Set `loot_mode=per_player` back. | [ ] | [ ] | |
| 3.8c | Crime: B steals from a stall or hits a townsperson in sight of a guard (B's own world). | B becomes wanted in B's world; A gets "<B> is now wanted by the guards (in their own world)"; A's world: no wanted state, guards ignore A. | [ ] | [ ] | |
| 3.8d | World event: when one starts near A (bounty hunters while A is wanted, reinforcements at a fort, a camp patrol), note it. | B gets "World event near <A>" with the event's name; the event's fighters near A appear for B and can be fought together. | [ ] | [ ] | |
| 3.9 | Chests: both open "the same" chest. | Both get the contents (each in their own world). | [ ] | [ ] | |

### 3b. Combat feedback (T36)

| # | Do | Expected | Pass | Fail | Notes |
|---|---|---|---|---|---|
| 3b.1 | B kills a bandit, then an animal, near A. | A sees on the right "<B> defeated an enemy" / "<B> defeated a Deer" in B's colour; B sees "You defeated ...". | [ ] | [ ] | |
| 3b.2 | B hits an enemy with a heavy attack and a kick while A watches. | The enemy staggers / is knocked back on A's screen as on B's. | [ ] | [ ] | |
| 3b.3 | B lights an enemy on fire / poisons it (if B has such a skill or arrow). | A sees the effect on the enemy. Note what does not show. | [ ] | [ ] | |

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
| 4.4c | Respawn option: both set `respawn_at=partner` under `[coop]`. B dies far from the checkpoint and presses U. | B gets up at the checkpoint, then is hopped back beside A ("Back to <A>"). Set `respawn_at=checkpoint` back. | [ ] | [ ] | |
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
| 6.9b | Race (T41): A (host) sets a map waypoint 300 m away (right-click on the map); A picks "Race to the waypoint", B accepts; both ride there. Then A sets a new waypoint. | 3, 2, 1, Go!; the banner shows both distances; the first within 12 m gets Victory on both screens with both times; a new waypoint starts round 2. | [ ] | [ ] | |
| 6.9c | Scoreboard (T43): after a few rounds of any challenge, both press F8. | "Record vs <other>: duels 2-1, ..." matches on both (mirrored); after 3 wins the winner's name on the other's partner frame reads "<name>, the Sparring Partner". | [ ] | [ ] | |
| 6.10 | Shift+F6 on an invite; Shift+F5 during a challenge. | The invite is declined; the challenge ends with the final series. | [ ] | [ ] | |

## 6b. Other mods (T28)

| # | Do | Expected | Pass | Fail | Notes |
|---|---|---|---|---|---|
| 6b.1 | Each player installs the mods they normally use (docs/compatibility.md), starts the game, loads in. | A notice "Other mods next to the game: ..." names them (or no notice with none); `cdmp.log` has a "Compatibility:" line. | [ ] | [ ] | |
| 6b.2 | Play 10 minutes of co-op with one fight and some loot. | Same as without the mods. If not: take the named files out, retry, note which mod. | [ ] | [ ] | |

## 6c. Story sync (new)

B should be behind A in the story (or use any two different saves). Both: note your quest journal's main quest first.

| # | Do | Expected | Pass | Fail | Notes |
|---|---|---|---|---|---|
| 6c.1 | B (guest, connected): F8, Down to "Copy my friend's story into my save (guest)", Right. | Within a few seconds B's panel: "Story from <A> received (N missions done; yours: M)". A's panel: "Your story went to <B>". | [ ] | [ ] | |
| 6c.2 | B picks "no", then "yes" again. | "Story copy cancelled", then received again. | [ ] | [ ] | |
| 6c.3 | B quits to the desktop, starts the game, presses Continue. | A notice "Your save now follows <A>'s story"; a folder before-story-sync-<date> in Documents/CrimsonDesert-CoopPlus-SaveBackups. | [ ] | [ ] | |
| 6c.4 | B opens the quest journal and the map. | B's main quest is A's; B's character, level, items, money and position are B's own. Note anything odd in the world (people or places that did not follow). | [ ] | [ ] | |
| 6c.5 | B and A play the same quest step together. | It advances for both. | [ ] | [ ] | |
| 6c.6 | Undo (optional): game closed, B copies the backup's slot folder back. | B's old story is back. | [ ] | [ ] | |

## 7. Ending

| # | Do | Expected | Pass | Fail | Notes |
|---|---|---|---|---|---|
| 7.1 | B leaves (Ctrl+F2). | A's world is normal (no leftover copies); B's own enemies are back; B's time and weather are B's own again. | [ ] | [ ] | |
| 7.2 | A quits the game while B is still connected (redo 1.1 first). | B gets "<A> left" and is back in their own world with no leftovers. | [ ] | [ ] | |
| 7.3 | Both quit WITHOUT saving. | | [ ] | [ ] | |

## Afterwards: what to send

From each PC: `bin64\cdmp.log`, the newest `bin64\cdmp_packets_*.log/.bin` pair, and this file with the boxes
ticked. "Diagnose Co-op+.cmd" puts a report on the Desktop that includes the log. Then set `record=0` again.
