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

## T04 -- Reconnect and rejoin
Plan: one-PC kill-and-rejoin loop (echo peer killed = the partner crashes; started again = rejoin), with and without
enemy copies; long alt-tab; check one spawn per connection, removal on each loss, no duplicate copies, no stuck UI.
Findings (build/port3479/rejoin_test.sh, alttab_test.sh):
- rejoin1: 5 cycles, game alive, 6 connections = 6 spawns, 6 "Puppet: remove queued" on the 5 s timeouts.
- rejoin2 ([dev] zone_copy_test=1): each loss "13 copies of theirs removed", each reconnect sends 13 new ones (new
  keys), never more: no duplicate enemies.
- alttab1: 40 s in the background (Notepad focused): no timeout, session kept (the game keeps running unfocused).
- The partner's own crash/restart is a new process: clean by construction; Steam / room-code rejoin: PLAYTEST 1.6b-d.
No code change was needed. Notes: the peer in the earlier late1 run timed out the client while the game was paused in
a minigame without focus; normal play does not pause.
- Pushed T01-T04 (ab89c6b..16f0872) to phase-a/local-pc-setup.
- T05 note: at the title screen (save not loaded) a connecting partner spawns nothing and leaves nothing behind (dropin1, 3 connects).
- T05 finding (dropin2): every (re)connect logs spawn -> "nothing from the other player for 8 s; removing their
  character" -> "the other player is back; reloading" -> spawn again, within 2 ms: a silence timer left over from the
  previous session. With a menu open the game holds injected packets, so both spawns land together and the game
  refuses the second ("Actor 생성에 실패했습니다" = actor creation failed); the copy is then missing until the
  "gone from our world; loading it again" check respawns it ~10 s later (self-healing, but a needless double spawn).
- T05 fix: CoopLink::on_reliable (Spawn) now sets last_remote_ms_ (net/coop.cpp). dropin3: map / inventory / pause /
  combat drop-ins = 4 connections, 4 spawns, 3 removals (the 3 leaves), no "actor creation failed"; after each menu
  closes one partner copy stands beside Kliff (screens dropin3-*). In the combat case Kliff was killed by the bandits
  before the peer joined: it joined during the death screen, spawned once, no crash.
- T05 status [~]: menus, title screen (dropin1), save loading (every PEER=1 run), cutscene (late2: peer joined during
  the arm wrestling intro), fight/death verified on one PC; a boss fight needs a boss -> PLAYTEST 1.6e.
- Pushed T05 (7e4c13f).
- T06 FINDING (desync1, 09:59): "Enemy relay: wanted but not started (its crash guards did not install)" -- on some launches shared enemies silently never start (the zone request is answered 'we do NOT relay'). dropin3 (09:48) started fine. Investigating.
- T06 root cause so far: desync1's late init had 4 hook failures at once (layer crash guard, HorseDiag "pending
  actions hook failed" = the head bytes matched but safetyhook could not create the hook, minimap icon hook,
  CheckAIBattleType). Same exe as the good runs; 1 of 80 launches in build/port3479 logs. Fix: the containment +
  layer guards are tried up to 5 times, 2 s apart (mod.cpp late init), and every failure now logs the safetyhook
  error type / which head did not match, so a recurrence says why. (Not reproducible on demand: verify in logs.)
- T06 done. coop_sync.* (Activity 15 digests: u16 sent | u16 present; 3 off in a row -> resync = session drop +
  rejoin, at most one automatic per 2 min), coop.cpp drift check (copy drawn > 40 m from the reported position for
  6 s, not while seated -> reloaded there), Resync action Shift+F8 / View + RB. Tests: desync2 ([dev] desync_test=1):
  one copy removed behind the relay -> "13 sent, 12 here" x3 -> resync -> copies sent again; second induced desync
  held by the 2-minute limit; Shift+F8 worked. desync3 (desync_test=2, frozen 60 s, Kliff walked 94 m): "drawn 94 m
  from where they are; loading it again there" -> copy beside Kliff (screen desync3-after-auto).
- Pushed T06 (4ca9e45).
- T07 findings: (1) on Leave (Ctrl+F2) / Join the CoopLink was destroyed before the session reported a lost link, so forget_partner never ran: the copies of the other player's enemies and their waypoint pin stayed (fixed in ~CoopLink); (2) Leave was only read on the Steam transport (moved out of the steam block); (3) the partner's name tag stayed on screen after leaving (HUD not cleared when the network stops: fixed).
- T07 done. leave3: Ctrl+F2 with 13 zone copies -> "13 copies of theirs removed", partner removed, "Network stopped
  (left the other player's world)", "now hosting our own world", screen clean (no name tag). Host-quit path = link
  lost (rejoin2-d1): "Lost the connection to ...'s world", no name tag, copies removed. Hidden natives: both paths
  call suppress_stop(restore=true) (restore verified in the 11-01 suppress lab); weather returns to the guest's own
  20 s after the host's reports stop (D37). Two-PC check: PLAYTEST 7.1 / 7.2.
- Pushed T07 (59e1c6e).
- T08 findings (audit1, first run of the state audit): (1) after a Leave on the one-PC (UDP) transport re-hosting
  could not bind the port the echo relay holds; run_network returned "stop" and the WORKER EXITED, i.e. co-op off for
  the rest of the game (any failed network start did this) -> run_coop now retries every 10 s; (2) audit "host
  weather stages 1" and "duel 1" 30 s after a lost link: both from the one-PC test switches (weather_test followed the
  host without a link; challenge_test kept the challenge) -- the real paths end them; the test paths now end with
  the link too so the audit measures the real behaviour.
- T08 audit2: B clean. A "enemy copies sent 6": g_partner_epoch started at 0 and send_enemy reads 0 as "current
  partner", so a zone tick decided before the first reset still sent after it -> epoch starts at 1. C "host weather
  stages 1" after Leave: weather was only restored by the next session's loop (which on the one-PC transport did not
  start) -> run_network restores our own weather when it stops.
- T08 done. net/state_audit.* (partner, seat, hidden NPC, enemy copies here / sent, our enemies hidden, scaling,
  host weather stages, cutscene copy, duel, hunt) logged 30 s after every session end ("State audit 30 s after the
  session: clean (...)" or "LEFT BEHIND: ..."). audit3: A (zone copies + host rain, partner crashes), B (duel running,
  partner crashes), C (plain session, Ctrl+F2): all three clean. The save is never written by Co-op+; savecheck after
  every session confirms the user's saves equal the 2026-11-01 backup.
- Pushed T08 (a16ff4a).

## T09 -- Menus and pause
Plan: measure (menus while connected: drops? bursts? stuck?); define: a menu pauses only your own world; the other
world goes on with your character in it -> tell the partner (Activity 16, resend 3 s), their enemies do not target
you (IsTargetable hook), no relayed blow is applied to you while in a menu, name tag "in a menu"; test all menus.
Findings: menu1 (map / inventory / pause 20 s each): no silence removal, no link loss, one spawn, copy fine after each.
Done: menu2 (map open inside the camp): "the other player is in a menu" .. "left a menu", aggro "copy 0 attacks"
while in it, first hit 1 s after closing; menu3: all three menus detected (enter + leave). [~]: the partner's view
needs two PCs (PLAYTEST 3.5b). Note: in menu2 the bandits killed Kliff after the map, so inventory / pause there
landed on the death screen (re-run out of combat).
- Pushed T09 (49f2ce9).
- T10/T12 note: in the 11-01 bandit captures NotifyHandleDeadAck = victim | KILLER (0xB01001C9 killed by 0xA0100001); a kill is followed by AddSubLevelExperienceAck 2995 (01 00 10 A0 | i64 max | 00 | ...). XP exists per kill.
- T10 finding (killopt1): KillCheatReq option 0 = death at once (NotifyHandleDeadAck type byte 0x10); options 1/2/3 = death ~5.3 s later (type 0x0F); killer = our player in all four; NO AddExperienceAck / AddSubLevelExperienceAck for any option. A natural kill gives AddSubLevelExperienceAck (11-01 capture). => the guest's XP cannot come from KillCheatReq.

## T10 -- Shared-kill loot and XP for the guest
Plan: XP first (does a kill give XP? can the guest's twin kill give it?), then loot rules: [coop] loot_mode
per_player / host_only / first_come (first looter takes it; the other side's body emptied via RemoveLootableAck).
Done: loot_mode (config, loot_twin, enemy_relay, Activity 17 kLooted both ways). loot1 (echo, first_come): bandit
0x72 killed (KillCheatReq) and looted (LootingDropFromDeadBodyReq 2491 -> RemoveLootableAck) on the host side ->
0.1 s later the guest side emptied copy 0xB01F0017 ("the host looted the original ... first"). Guest-first and the
real guest need two PCs (PLAYTEST 3.8b). XP: 3 approaches failed -> BLOCKED.md. [~].
- Pushed T10 (ae487ea).
- T11 FINDING (stress1): FPS 70 in every phase (no cost), out 4.5-8.6 kB/s, no crash / silence; but enemy copies received in 90 s: good 12, bad (150 ms, 50 jitter, 5% loss) 8, awful (300/100/15%) 0, lossy (100/20/30%) 0. Investigating.
- T11 correction: the copy count in stress1 was from a rate-limited log line; counted by "copy ... exists" every
  phase loaded all 12-13 zone copies, 30 % loss included. Not a problem.
- T11 done: session byte counters + "out/in kB/s" in the Net log line; poor-link half rate (rtt > 250 ms or > 32
  reliable pending; held 5 s; back under 150 ms / < 8). Report: docs/tests/stress-report.md. Note: the relay answers
  pings itself, so rtt does not include its simulated delay (the backlog trigger is the one seen on one PC).
- Pushed T11 (55f5e00).
- Installed package 55f5e003 (P0 complete): Defender clean, cdmp.asi hash matches, shipped ini, test save out, SAVES OK.

## T12 -- NPC and wildlife consistency
Plan: decide the rule (copying animals / townsfolk too would double the relay's load and their AI is not worth
sharing; each world keeps its own, enemies stay shared), write it in the README, verify the contest's counting.
Findings: animals near the bandits save start (npcs.py): geese 15 m (0xB010007F/80), partridges, herons, Stefero;
all in animal_names.inc (hex keys). huntkill2: walking up and swinging hit nothing (birds fly off; a sword cannot
reach them). huntkill3 (no hunt_test bypass): picker -> hunting contest, KillCheatReq on 6 animals -> the death
notices name our player as killer -> "Hunt: we brought down Goose (+1), score 1" and "... score 2". The real-blow
attribution path (guest_hits_we_hit) is the PLAYTEST 6.6 step.
- Pushed T12 (736daaf).

## T13 -- Boss mechanics audit
Plan: table of every one-player assumption (from the mod's boss research and the guides), its risk with two, the
handling or fallback; first five bosses into the playtest. [needs-2PC].
Done: docs/research/boss-audit.md (12 mechanics); PLAYTEST 5.6b per boss. Only open risk: grabs / paired attacks on
the guest's copy (fallback enemies_target_copy=0). Kearush's fog is host-only (guest unaffected; accepted). [~]
- Pushed T13 (0ff8fad).

## T14 -- Partner party frame
Plan: stamina into PlayerState (protocol 12), ui::party_frame (gold diamond + serif name, red health bar, thin pale
stamina bar, status line), drawn whenever connected except over full-screen menus, cutscenes and our death screen;
under 25 % health the bar edge pulses red and the line reads "Badly hurt".
Done: frame3 (echo): normal (health ~60 %, stamina full, "2 m") and after our HP -140 via the dev script ("Badly
hurt - 2 m", red edge) -- crops in build/port3460/shots/frame3-crops.png. frame1: -260 killed Kliff (the frame stays
hidden under a death screen, as designed); frame2: the game did not start (Steam), retried.
- Pushed T14 (9204c57).

## T15 -- Teammate revive
Plan: net/coop_revive.* (bleed-out 30 s, reach 3 m, hold 3 s, Activity 18), revive_in_place (the boss-win path:
the server's checkpoint revive without the move = up where we fell, full health), ReviveFriend action (Shift+F7 /
View + LB), hint strip, credit notices, [coop] teammate_revive, [dev] revive_test for the one-PC loop.
Done: revive1 (one PC): Ctrl+Shift+F9 kill -> game's own death screen + "Hold to revive" -> Shift+F7 held 3.6 s ->
"we revived" -> echoed -> "revived us: up where we fell" -> checkpoint revive steps 1-3 (no move) -> "alive again",
hp 300/300 at the death spot; notice "You revived ...". Boss fights keep boss_death_wait. [~] (PLAYTEST 4.4b).
Note: on this run the game's OWN death screen appeared (not Co-op+'s); fine either way.
- Pushed T15 (80e2881).

## T16 -- Crime, guards and bounty
Plan: confirm the guest cannot commit crimes in the host's world (only enemy copies take the guest's blows), see
what a crime sends, notify the partner, and a shared option if the crime can be put into the other server.
Findings: crime1 (pub, no weapon drawn in town: no crime); the test save already has a wanted record at login
(WantedRegionDataListAck: player | region 0x000F42C3 | ... | price). Crime opcodes: WantedAddCrimeRecordReq 2836 /
Ack 2569, ChangeWantedStateAck 2324, IncreaseWantedPriceAck 2249, SetWantedForDevReq 2212.
Done: activity_observe_s2c -> Activity 19 kWanted (once a minute) -> notice; wanted1: an injected
ChangeWantedStateAck -> "cdmp_peer (relay) is now wanted by the guards (in their own world)", game fine. Shared
bounty -> BLOCKED.md. [~] (PLAYTEST 3.8c with a real crime).
- Pushed T16 (ec5be27).
- T17 finding: guest lethal blows already become the HOST's normal kills (VaryStatCheatReq to 1 HP + our collision: quest credit, skill XP, loot stay with the host). Lead for T10 XP noted in BLOCKED.md.

## T17 -- Anti-grief protections
Plan: list what the guest can reach in the host's world (only copies of enemies take its blows; townsfolk, items,
objects are per world), check quest credit of guest kills, add a strict switch.
Findings: guest lethal blows already become the HOST's normal kills (VaryStatCheatReq to 1 HP + host collision):
quest credit stays with the host. Bug fixed: with the lethal clamp on (bosses), the non-lethal branch applied the
full damage and could still take the enemy to 0 -> now capped at 1 HP. Added [coop] guest_finishes (default 1;
0 = never finish any host enemy) + profile lines (also loot_mode, teammate_revive).
grief1: no relayed blows landed (Kliff's swings never connected with a copy on one PC), so the clamp itself is for
the playtest (PLAYTEST 3.5c). [~]
- Pushed T17 (1a0b9b6).

## T18 -- Trading and gifting  [!] blocked
Plan: find the giver's remove and the receiver's add requests; transaction with confirmation and a log.
Findings: reqsizes.py on the item requests (AddMinusItemReq [u32, 8], DiscardItemReq [u16, u16, 8],
DeleteItemCheatReq [u16, u8], RemoveItemToInventoryReq [u8, u16, u8], CreateItemFromTrItemValueCheatReq: a
structure); addminus1: AddMinusItemReq +1 / -1 on item 22007 -> no answer, no inventory change. No way to add a
chosen item on the receiver -> BLOCKED.md (with what would unblock it).
- Pushed T18 notes (9a710ed).

## T19 -- Party travel and summon
Plan: consent-based on top of the existing go-to (Ctrl+F4 hops, beside the other player, not under ground):
Summon action (Shift+F4 / View + R3, 30 s cooldown) -> Activity 20 -> "<name> asks you to come: {GoToFriend} go"
for 20 s; our own long trip (> 250 m in 5 s) -> "travelled far away: follow them". Auto catch-up: not added (the
offers cover it without moving anyone unasked).
Done: summon1 (echo peer offset 60 m): "You asked ... to come to you", request line on screen, cooldown notice,
Ctrl+F4 moved Kliff 60 m to "them" (an artificial spot inside a cliff -> he ended on the cliff face; a real friend
stands on ground). Party-travel offer not exercised on one PC (needs a fast travel). [~] PLAYTEST 2.7b.
- Pushed T19 (7b57325).

## T20 -- World-state persistence rules
Plan: the per-world design decides most of it; write the table (kills, bosses, gathering/chests/doors, quests, time
and weather, crimes, scaling) for host and guest, README section, and point at the tests that show each outcome.
Done: docs/world-state-rules.md + README "WHAT STAYS AFTER A SESSION". Evidence: audit3 (guest's hidden enemies
back, nothing left), loot1 (the guest's stand-in enemy killed for its drop stays dead), leave3, boss audit. [x]
- Pushed T20 (c3e3a74).
- T21 finding: in the empty-table runs (peer from the start) the table mercenary 0xB010035C runs StartAIFunction 0x67ED3753 (and barely moves: 2 actions vs 106); staffed (no peer) it runs 0xC7A77DBA and sits. The AI reacts to the copy. Testing: copy untargetable for everyone.
- T21 ROOT CAUSE: our client tells its own server when the COPY enters triggers: GamePlayTriggerInfoEnterReq 2849 (01 F0 10 A0 | trigger 0x000F4280 | 01) at 126.944 and the table mercenary's AI switched (StartAIFunction 0x67ED3753, no more sitting) at 127.108; also SequencerTriggerVolumeInOutReq 2620 (copy key at +13). Fix: those messages naming a copy never reach our server.
- T21: the scrub fires (vs4) but the table was empty again at 10:27; staffed runs with the peer were at 10:33-10:35 (aweye1, pw4); without the peer staffed at 10:27 (tt1). Actor keys are per load (0xB010035C was a gimmick in vs4), so the AI comparison above is not reliable. New suspect: time sync (only with a partner) re-evaluates NPC schedules. Testing time_sync=0.
- T21: the copy also bumps pub gimmicks on arrival (UpdateGimmickReq 2787 / GimmickExecuteFrameEventReq naming 01 F0 10 A0 as user: 0xB010030C/30D/31A/35B/35C at 127.2 s); seats may count as used. Trying: such gimmick messages get gimmick key 0.
- T21 objective check (table_check.sh: full circle of R presses, usable = InteractionDoStepReq): with the peer from the start 0/3 (tc1-3, after both scrubs), without 1/1 (tcn1, turn 0). Next: is it the copy standing 2 m from us (interaction focus / blocking)? offset 6 m.
- T21: tcnc (peer connected, [dev] no_copy=1): TABLE USABLE at turn 0 -> the copy's presence is the cause (not time / weather / relay).
- T21: tclate3 (copy loaded AFTER arriving at the table): USABLE -> the problem is the copy present while the pub loads around us. Remaining client messages naming a copy (tco6 capture): RegionInOutReq 2539 (copy body 0xB010F001 entering regions 0x1EB29 / 0x127C5 at the pub arrival), CurrentTargetChangeReq (NPCs targeting the copy: kept, aggro needs it). Adding RegionInOutReq to the scrub.
- T21: tcr1 (region scrub too) and tcu1 (copy untargetable) both still unusable. Next: does a resync (copy unloaded and loaded again after arriving) clear it?
- T21 [!]: resync after arrival (tcrs1) does not clear it either. Five approaches in BLOCKED.md. The scrubs stay
  (our server should not hear about copies in its triggers / regions / gimmicks anyway). Real two-PC play may
  differ (the echo copy teleports in with us).
- Pushed T21 (7541ce4).

## T22 -- World event sync
Plan: events = stages our server starts (PrepareSequencerAck); their fighters are enemies = already shared near the
host. Add the "it is happening" signal: tools/gen/event_names_inc.py (519 Reinforcement_ / WantedChaser_ / Node_
stages, readable names) -> host-side PrepareSequencerAck watch -> Activity 21 kEvent -> notice; [dev] event_test.
Findings: no event stage occurred in today's captures (start area, camp, pub), so natural events are for two PCs.
Done: event1 ([dev] event_test=1001215): "World event near cdmp_peer (relay) / Bounty hunters (Tashkalp, level 1)".
[~] PLAYTEST 3.8d.
- Pushed T22 (8486e81).

## T23 -- Object interaction sync  (deferred, revisit if time remains)
Findings: objects are per world; static scene objects share scene ids across worlds (the rope / tree feature
locates trees by scene id, remote_puppet.cpp g_scene_gimmicks). Plan: relay the host's persisted object states
(UpdateFieldGimmickSaveDataKeyAck: door opened, lever pulled, destroyed) by scene id and inject them into the
guest's client (visual only; the guest's server keeps its own); needs a door / lever near a test save to read the
layout. Not started: lower value per hour than T24-T40.
- T24 BUG found in testing: after chatting, Ctrl (for Ctrl+F3) triggered LEAVE (Ctrl+F2): F2 counted as held (half of its press swallowed by the hook). Fixed: the opening Shift+F2 is never swallowed (seen only); typed keys are swallowed down and up together.

## T24 -- Communication
Plan: net/coop_chat.* (Activity 22: u8 kind | text), typed line through the activity keyboard hook (keys kept from
the game while open), quick list on the Ping action (pad usable), notices + chat log (ui::chat_log), "here" quick
messages drop the old map pin.
Bugs found in testing and fixed: (1) the line reopened after sending and (2) Ctrl then triggered LEAVE (Ctrl+F2):
the hook had swallowed half of the F2 press so F2 stayed "held". Now the opening key is only seen, never
swallowed; typed keys are swallowed down and up together; the line opens from the hook's own Shift+F2 event.
Done: chat7: typing line, "cdmp_peer (relay): hello friend" notice + log, quick list (gold choice), "Danger here!"
sent + pin. Pad: same actions (View + D-pad Left opens, D-pad Left/Right choose), not driven by a pad here.
- Pushed T24 (2edf0f4).

## T25 -- Mount edge cases
Plan: echo horse runs (horse_test.sh, AGAIN=1 for a second summon / mount / dismount); flying mount checked earlier
(D43, dragon tests). Death / swapping need two horses / a fight: playtest.
Finding (mount1, mount2): after a dismount the rider reloads "the horse stays", but the horse copy no longer existed,
so a new copy was prepared from the old spawn data -- at the spot where the horse was first called (-10340, -3928)
while the real horse stood at (-10290, -3990). Fix: the sender's horse position is tracked from its Move /
StartAction (horse_event) and a new horse copy is placed there; forgotten when the horse copy is removed. Also the
kept copy is reused whenever it exists (the key check was not the failing part, kept for safety).
mount3: reload placed the horse copy at (-10265.8, -4017.6), real horse at (-10267.4, -4017.4) (+2 m echo offset).
The second summon in mount1 never reached the horse (Kliff stuck between rocks). [~] PLAYTEST 2.3b.
- Pushed T25 (766a5ae).

## T26 -- Death penalty and respawn options
Plan: penalty options need an item / silver removal the server accepts (none found, T18) -> BLOCKED; respawn option:
[coop] respawn_at=checkpoint|partner -- after any revive that put us more than 50 m from where we fell, 3 s after
getting up, the go-to hop to the other player.
Done: respawn1 (walked away, killed, U): checkpoint revive steps 1-3, "at the checkpoint", then "respawn_at=partner:
from the checkpoint to the other player". Profile line, README, PLAYTEST 4.4c. [~]
- Pushed T26 (63d9d7f).

## T28 -- Mod compatibility testing
Plan: no downloads of other authors' mods, so (1) list the kinds of mods in use (nexus survey) and what each means
for co-op, (2) at launch look in bin64 for other ASI loaders (proxy DLL names), other .asi files and ReShade, and say
so in the log and once on screen (never blocking), (3) guard against Co-op+ being loaded twice (two loaders),
(4) test both cases with a harmless ReShade.ini stand-in.
Finding (compat-a, 15:43): the game crashed 2 s after "Crash guards: try 1 of 5 failed". Symbolised (dbghelp,
cdmp.pdb): horse_diag.cpp:377, `g_ret[0] = 0xC3` -- the retry (T06) wrote into the return stub the first try had made
PAGE_EXECUTE_READ. remote_puppet's g_ret_stub had the same pattern. Both now written once, at allocation. The T06
retry had never actually been exercised before (it fires on ~1 launch in 80).
Also: compat_test first passed a /c/... path to powershell -Command (game never started); fixed with a Windows path.
Done: compat-a (ReShade.ini present): "WRN Compatibility: next to the game: ReShade / a dxgi.dll proxy..."; compat-b:
"Compatibility: no other ASI loader, .asi mod or ReShade next to the game". docs/compatibility.md; PLAYTEST 6b. [~]
(real third-party mods are a 2-PC/user test.)
- Pushed T28 (eb53dd5).

## T27 -- Nexus release package
Plan: README sections for manual / Vortex / MO2 installs, other mods, known issues (from BLOCKED.md and the [~]
items); BUG-REPORT.txt and OTHER-MODS.md (docs/compatibility.md) into the zip; a second zip with the bin64 layout for
mod managers; the Nexus page rewritten for 1.0.0.3479 and today's features (docs/release/nexus-page.md, moved from
docs/drafts); a shot list; a clean-machine walkthrough on a fake game folder.
Done: tools/test-install-fake.ps1 (temp "Crimson Desertin64" with a stand-in exe, another mod's winmm.dll and a
stand-in save): zip contents + SHA256, install (files, the other winmm.dll kept, saves backed up, version note),
update keeps an edited cdmp.ini (defaults as .new), vanilla on/off, uninstall leaves bin64 byte-identical to before,
backups left alone, mod-manager zip lands next to the exe -- 13/13 PASS (eb53dd51 test package).
Not done here (needs the user): the actual Nexus upload, screenshots (shot list) -- FLAGS. Vortex itself is not
installed on this PC, so the Vortex lines are generic ("deploy into the game folder"). [~]
- Pushed T27 (403e466).

## T29 -- Error messages
Plan: list every failure state (mod, installer, uninstaller, diagnose) with what the player sees; give the ones that
were log-only a plain message with the next step; one mechanism: render::hud_set_problem(kind, text) -> red lines at
the top of the F8 panel, and a strip at the top of the screen for 15 s whenever a line changes.
Found log-only before: network not starting (T08 retry loop), save backup failure, crash guards off (enemy relay
dead for the session), a bad room code, a guest that never reaches the host (panel said only "looking for the other
player..."), Steam never logging in. Old protocol message was jargon ("uses protocol 11, we use 12").
Done: errors_test (build/port3479): A guest with no host, 60 s after load: panel "Can't reach the host yet. Check: both
games running with a save loaded, the same Co-op+ zip, Steam online. Still stuck: Ctrl+F2, then Join Game again.";
B stand-in host joins: the line is gone; C stand-in drops: notice "Lost the connection to cdmp_peer (relay)'s world.
Reconnecting by itself; [Ctrl][F2] goes back to your own world" (key glyphs drawn). docs/error-messages.md. [x]
- Pushed T29 (ca3eeee).

## T30 -- Settings and hotkeys
Plan: one settings block at the bottom of the F8 panel (settings_menu.cpp): arrows (keyboard hook, swallowed while
the panel is open) / D-pad without chord (masked from the game) pick and change; each change is written with
WritePrivateProfileString (keeps cdmp.ini UTF-16) and applied live through the modules' setters where they exist
(loot mode, guest finishes, revive, respawn, boss wait, weather, scaling); join and nameplates say "(next start)".
Hotkeys: the bindings table is no longer const; [keys] <Action>=Ctrl+F1 / <Action>_pad=Y; warnings for unknown
names, two co-op actions on one key or pad button, and plain keys (no Ctrl/Shift, not an F-key).
Done: settings_test (build/port3479): [keys] Invite=G -> "Keys: Invite is on G with no Ctrl/Shift: the game probably
uses that key too" (log + notice); F8, Down, Right -> "Settings: Loot of shared kills = host only", ini
loot_mode=host_only; Down x7, Right x2 -> enemy health x1.5 -> x1.7, ini panel_health_2p=1.7; panel screenshot shows
the block with the gold selection. Shipped ini restored after. README. [x]
Not done: a full in-game key-capture rebinding UI (keys are rebound in the ini; the panel says where).
- Pushed T30 (522a3db).

## T40 -- In-game difficulty controls (done with T30)
Plan: two settings rows for the 2-player multipliers (regular, boss), live through new party_scaling setters
(scaling_set_enabled, scaling_set_2p), stored as [scaling] panel_health_2p / panel_boss_health_2p, read at start
after scaling.toml so the panel's value wins; the on/off switch too.
Done: settings_test: enemy health x1.5 -> x1.7 live ("Settings:   Enemy health with a guest = x1.7"), saved in the
ini. 3-4 player values stay in scaling.toml (2 players only today). Applied on the host (it is the host's world). [x]
- Pushed T40 (5da9e59).

## T31 -- First-run onboarding
Plan: a new Help action (Shift+F1 / chord + L3; chord + L3 now masked from the game like R3); ui::guide_card (game
dialog style, action tokens drawn as key / pad glyphs); shown once at the first in-world snapshot when cdmp.ini has
no [general] onboarded=1 (then written), toggled by Help, closes itself after 60 s or when a game menu opens.
Done: guide_test run 1 (25 s load wait) toggled the card during the loading screen (fine, but too early for the
first-run check); run 2 (55 s): first shot in the world shows the card ("Co-op+: first run: controls card shown"),
Shift+F1 closes, Shift+F1 reopens; ini onboarded=1 after. Shipped ini restored. README / Nexus controls. [x]
- Pushed T31 (4e3e2bc).

## T32 -- Connection quality indicator
Plan: every datagram carries the sender's packet number; ClientSession counts the gaps (stats.lost, a late one taken
back); mod.cpp turns lost / received over ~3 s into a loss %; panel line "loss x%" coloured; [overlay] net_hud (F8
settings row "Ping and packet loss on screen") draws "Co-op 28 ms 0% loss" top right, green / yellow / red; the Net
log line gets "lost N".
Finding (netq1): with a stand-in that reconnected, the loss stayed 0: the new peer numbers from 1, and my first test
("seq + 1000 < last" = restart) took 1..800 as late packets. Now anything more than 64 back is a new numbering.
Done: netq2: clean (25 ms delay) "Co-op 28 ms 0% loss" green; 10 % relay loss: the log's lost count grows ~8-11 per
5 s (~10 %), panel "loss 6.9%" yellow, readout "33 ms 7% loss" yellow. [x]
- Pushed T32 (816a660).

## T33 -- Partner audio
Plan: the partner copy is a game character driven by the game's own Move / StartAction events, so its animation-
driven sounds (footsteps, swings, horse) should come from the game's audio engine with the game's 3D falloff. Check:
measure what the PC plays (new tools/audio/loopback_rms.exe, WASAPI loopback, high-passed loudness and onsets) while
the copy walks next to a still player (relay with a 6 s delay replays our walk after we stop) vs no copy.
Result (audio_test, build/port3479): A (no copy) median -78.4 / -76.7 / -77.2 dBFS, max -73; B (copy walking 2 m
away) median -74.6 / -76.8, max -68 / -69. Slightly louder with the copy, but the game's whole output reaches the
loopback at about -77 dBFS here (near silent: the game or its device is turned down on this PC), so this does not
prove it. No sound is added by the mod; voice lines of the player character are not expected for the copy.
Status [~]: PLAYTEST 2.7c (headphones, distances 2 / 20 / 60 m). If the copy is silent there, the next step is to
find the anim-notify sound path for network-created actors.
- Pushed T33 (b77a904).

## T34 -- Per-player colours
Plan: one colour and one mark per role, the same on both screens: host gold + diamond, guest blue + round mark (the
mark makes them read apart without colour); colour-blind-safe pair from Okabe-Ito (orange 230,159,0 / sky blue
86,180,233). game_ui::set_partner(colour, round) used by the name tag, the partner frame and the partner's chat lines.
The game's map pin colours are its own palette (15 entries, not documented): a settings row picks the friend's pin
colour (live through puppet_map_pin_style). Pings use the same pin.
Done: colours_test: as guest the host's tag + frame gold; F8 > Player colours > colour-blind safe ("Settings: Player
colours = colour-blind safe") -> orange, live. The host's view (blue, round) and the pin colour need two PCs:
PLAYTEST 2.7d. [x]
- Pushed T34 (d255c2a).

## T35 -- Emotes and resting together
Plan: find the game's emote / sit / rest systems and sync what the copy does not already show.
Finding (web search 2026-11-03, powerpyx / dexerto / games.gg guides): no emote or gesture system is documented for
Crimson Desert; resting is Focus (Ctrl / LB) on a campfire or cooking pot > Wait (skips time, with a cooldown), or a
bed. Sitting on benches / chairs and the rest are interactions that play the player's own actions -- the copy is
driven by every StartAction we send (the same path as climbing, gliding, the arm wrestling seat), so the friend sees
them. Wait changes the time of day: the host's Wait moves the guest's clock through time sync (T20 rules); the guest's
own Wait is overridden by the host's time, which is the defined rule (time follows the host).
Nothing new to build without an emote system. Not tested on one PC (no campfire near the test saves; the echo copy
mirrors our own actions anyway). [~] PLAYTEST 2.7e.
- Pushed T35 (a649bcd).

## T36 -- Combat feedback sync
Plan: what the partner's hits do to enemies is already real in the host's world (guest_hits sends the game's own
DoFrameEvent / damage, with stagger, T17), so stagger, knockback and the enemy's reactions are the game's; the game
shows no damage numbers. Missing: a kill feed. Built: net/kill_feed.cpp: our kill = a death notice naming us as the
killer, or one of our blows on the victim in the last 3 s (the hunting contest's rule); a line "You defeated a Goose"
and activity type 23 with the name, so the friend sees "<name> defeated a Goose" in our colour (right side, 6 s, 4
lines). Names: the hunting table's animals, "an enemy" otherwise (no enemy name table yet).
Done: killfeed_test: KillCheatReq on 6 animals near the start: "Kill feed: we took down 0xB010007F (Goose)" x4,
"(an enemy)", "(Scolt)"; the screen shows the echoed lines "cdmp_peer (relay) defeated a Goose ..." in gold.
Not done: per-player lock-on (the game's lock-on is local; nothing to sync found), status-effect check, enemy names.
[~] PLAYTEST 3b.
- Pushed T36 (9ddd859).
- Pushed T37 (a069441).

## T37 -- Crash and log reporting
Plan: Diagnose writes the report as before plus a .zip (report.txt + the last 50 000 lines of cdmp.log) and puts the
.zip on the clipboard (Set-Clipboard -Path); everything passes a Redact step: Steam names learnt from the log's own
lines ("playing as", "connected to", "while '...' is in the session", player_name=) -> <player1>/<player2>,
SteamID64 -> <steamid>, lobby ids, room codes, C:\Users\<name>, %USERNAME% / %COMPUTERNAME%, IPv4 (not 127.x), e-mails.
-NoRedact keeps everything.
Findings while testing (tools/test-diagnose-redact.ps1, a fake log full of personal data): (1) Python had turned
"\b" into backspace characters in my regexes -> SteamID, room code and IP leaked; rewritten from a file. (2) "Room:
hosting" came out as "Room: <code>" (case-insensitive 7-letter match); codes are upper case, now matched so.
Done: test: 9/9 secrets gone, game version 1.0.0.3479, coordinates and 127.0.0.1 kept; real install: report + zip
written, no Windows user / PC name in them. [x]
(The first T37 commit a069441 missed these notes and the README lines: a Python escape error; added in the next.)
- Pushed T37 (a069441 code, notes in the next commit).

## T38 -- Post-patch playbook
Plan (drafted with T21, commit 7541ce4): docs/post-patch-playbook.md (keep the old exe, what changed, re-find what
moved with the build/port3479 tools, the order to bring features back, the tests) and tools/port/post_patch_check.ps1
(runs upstream's verify-port.ps1 against the installed exe and lists every hook / offset problem in the last cdmp.log).
Done: on the installed 1.0.0.3479: "RESULT: ALL OFFLINE CHECKS PASSED", "no hook or offset problems logged".
A copy of the exe with 4 bytes changed at kClientPostPacket (exe+0x3A5A30) and kActionLayerAdvance (exe+0x214C070):
"FAIL kClientPostPacket ... expected 48 89 5C 24 10 4C 89 4C, found CC CC CC CC ...", "FAIL kActionLayerAdvance",
"RESULT: 2 CHECK(S) FAILED" -- exactly the two, named with the feature each one guards. Copy deleted. [x]
- Pushed T38 (bca36e4).

## T39 -- Story spoiler protection
Plan: a per-player setting (F8 settings row "My friend's story": shown / hidden, [coop] spoiler_guard), live:
(1) party journal: a friend's quest step that is not one of ours (not under way, not done) shows "A quest you have
not reached yet (name hidden: spoiler guard)" -- not even the log names it; (2) stage cinema: the friend's story /
boss scene is not started in our game (their "is watching a cutscene" notice stays). Being pushed ahead cannot
happen: quests are never changed in the other save (D42).
Finding: in coop_activity's event switch, `case kMenu:` (T09) sat right under `case 7/8/9, kDuel, kQuest:` which
had ended in `break` -- so cutscene, duel and quest events ran the menu code and set "partner in a menu" (copy
untargetable, no damage) from their first data byte. No false "in a menu" line found in later logs (those bytes were
usually 0), but it is fixed (break added).
Done: spoiler_test: dev quest_test "completes" Reconstructing Pailune (1000230) and the stand-in echoes it: notice
"cdmp_peer (relay): Quest Updated / A quest you have not reached yet (name hidden: spoiler guard)". The scene part
needs a host at a story scene: PLAYTEST 2.7f. [x]
- Pushed T39 (f70661b).

## T41 -- More challenges
Plan: of the five (horse race, fishing contest, boss rush / arena, hide and seek, scavenger hunt) the race fits the
existing pieces best: the challenge picker and series, the shared map waypoints, the hunting contest's round state
machine. net/coop_race.cpp: finish = the host's waypoint, else the guest's (both known on both sides: our own from
CreateOrChangeSelfDestinationReq in local_capture, theirs from the Waypoint event in coop.cpp; pings ignored), so no
extra message picks it; 3-2-1, first within 12 m (2D) sends its time (Activity type 24), the other has 60 s, then the
result; a waypoint 30+ m from the last finish starts the next round; banner with both distances.
Done: race_test: picker -> Race (option 5), challenge_test accept, dev script sends our waypoint 60 m east
(c2s 2663): "Race: round 1 to (-10280, -3915)", walked there: "we finished round 1 in 0:27.9", the echo "the other
player finished ... 0:27.9", "round 1 ends"; screen: banner "Race to the waypoint vs ... 0:03.2 you 54 m ...",
"Victory / You 0:27.9 them 0:27.9", toast "You win the race. Series: you 1 - 0". [~]
Not done: fishing contest (no fishing hook found yet), boss rush / arena (bosses are the host's story stages),
hide and seek, scavenger hunt. PLAYTEST 6.9b for the race.
- Pushed T41 (7d8efca).

## T42 -- Duel extras
Plan: of wagers, selectable arenas, spectator camera, rematch: (1) arenas = the duel ground's size, a settings row
([coop] duel_ring_m 12 / 20 / 35); the side that places the ground already sends centre + radius, so both use the
placer's size; (2) rematch = the picker remembers the last game and opens on it (Shift+F5 twice). Wagers need an item
or silver transfer the game server accepts (none found, T18 / BLOCKED.md); a spectator camera needs control of the
game camera (not hooked).
Done: duelx_test: [coop] duel_ring_m=12 -> "Duel: round 1 at (...), radius 12 m"; after ending, the picker opened on
Duel (Duel is also the first option, so this shot does not prove the rematch memory; the code is 4 lines: the
chosen stage is kept and the picker's selection set to it). [~]
- Pushed T42 (f839954).

## T43 -- Cosmetics and recognition
Plan: titles + a scoreboard (co-op bounties left out: bounties are the game's own and per player, T16). Every change
of the challenge series counters (any game) is a round won or lost -> net/scoreboard.cpp writes it per friend in
cdmp_scores.ini next to cdmp.ini (section = the Steam name made safe) and a _total section; the panel shows "Record vs
<friend>: races 1-0, ..." and "Your title: ..."; titles from totals (3 wins the Sparring Partner, 25 the Champion,
10 duels / races / hunts the Duelist / the Swift / the Huntmaster) go to the friend (Activity type 25) and appear after
the name on their partner frame.
Done: scores_test (the race test with a scores file seeded at 2 wins): race won -> "Scoreboard: won race vs cdmp_peer
(relay) (race 1 - 0)"; panel "Record vs cdmp_peer (relay): races 1-0", "Your title: the Sparring Partner"; partner
frame "cdmp_peer (relay), the Sparring Partner" (our title echoed back). Test scores file deleted after. [~]
(co-op bounties not built; the panel is now long with the settings block -- it can cover the partner frame while
open.)
- Pushed T43 (3edc116).

## T44 -- Group photo mode
Finding (web search 2026-11-03, thegameswiki photo-mode page): the game has a photo mode (beta) in the pause menu that
pauses play and frees the camera. The friend's copy is a real game actor in the scene, so it is in the photo as it
stands; the pause freezes it too. What co-op needs is the pose: a ninth quick message "Hold still: photo!" (the quick
list is now sized by its array, not a fixed 8). Our overlay already hides over game menus (ui_menu_open), and now the
partner frame hides while the F8 panel is open (it overlapped the long panel, T43).
Not tested on one PC (the photo mode menu entry was not driven by keys here; the echo copy mirrors us). [~] PLAYTEST 2.7g.
- Pushed T44 (353290d).

## T45 -- Shared stash: [!] (BLOCKED.md): same missing add-item request as T18 (three approaches there); a mod-side
stash would delete items. T46 -- Baby wyvern: stays [!] (pet only in this version; F7).

## T47 -- Voice chat: not started (deferred)
Low priority by the roadmap's own words (friends use Discord). Plan for later: Steam voice through the same
steam_api64 the mod already resolves at runtime (ISteamUser::StartVoiceRecording / GetAvailableVoice / GetVoice on the
sender, DecompressVoice + a waveOut stream on the receiver, push-to-talk on a [keys] binding), sent as an unreliable
event type over the existing Steam transport; volume falloff by distance like T33. Needs two PCs to test at all.

## T48 -- 3-4 player support: not started (deferred)
The architecture is two-player throughout: ClientSession has one peer, the puppet is one copy (kPuppetBody /
kPuppetHorse keys), the enemy relay keys copies by "the partner", challenges and revive assume one other player,
the party frame shows one friend. Scaling already has 3-4 player curves (scaling.toml). Doing it without destabilising
2-player play means a session per peer and N puppets (new reserved actor keys) -- a project of its own; the roadmap
allows skipping it ("only if the architecture allows it").

## Regression run on the evening build (2026-11-03, after T48; ROADMAP section 2)
- Shared enemies + host weather + duel + lost links + leave, each followed by the 30 s state audit (reg-audit):
  21 enemy copies relayed and removed; three audits "clean"; no crash.
- Rejoin x5 (reg-rejoin): spawn 1..6, one character per spawn, no crash.
- Menus (reg-menu): map / inventory / pause each "in a menu" then "left a menu"; spawns 1, links lost 0.
- Teammate revive (reg-revive) and the death screen (reg-qu): Q -> hp 108 (instant), U -> checkpoint revive steps
  1-3, "at the checkpoint", hp 300.
- Horse (reg-horse): ride, gallop, dismount, the horse copy reloaded where the horse is; the script's optional second
  round hung walking to the horse (test script, not the mod); stopped.
- Controls card with a pad (guidepad, [dev] pad_sim=1): pad glyphs (View + Y, B, RB, LB, RS, LS) on the card.
- Vanilla switch on the real install: vanilla.ps1 on -> cdmp.asi.off, game started, no cdmp.log written; off -> back.
- Not covered on one PC: Steam invite / room code (two accounts), a real controller.

## User request 2026-11-03 18:00 -- controls card with controller buttons, a playtest Discord, a feature list
- Controls card: input::connected_pad() (XInput slot answering, or a PlayStation HID reader running); the card draws
  that pad's buttons whenever one is plugged in (not only after it was used last); the pad text skips the keyboard-
  only chat line and says "Hold View / Touchpad for every co-op button"; the F8 panel names the card's key.
- Distribution: decided GitHub Releases on a separate PUBLIC repo that holds only the downloads (the project repo
  stays private): a stable "latest" link for Discord, versions with notes, SHA256, no size limit; source.zip in the
  zip now holds only the build sources (GPL) -- no notes or logs (it was `git archive HEAD` of everything).
- docs/release/FEATURES.md (Reddit, tables) and docs/release/discord-features.txt (3 messages under 2000 chars).
- Final build 7a880b02 packaged, scanned (Defender: no threats), fake-folder install 13/13, installed; savecheck OK, test save out, shipped ini; publishing left to the user (F12). FINAL-REPORT.md.

## Discord server set-up (user request 2026-11-03 18:30, server "Seamless co-op Crimson Desert")
Done in the user's Chrome (signed in as Asher):
- Categories: Start Here (top) -> #announcements, #how-to-install, #features, #download-here (moved in);
  Text Channels -> #general, #clips-and-highlights (unchanged); Playtesting -> #bug-reports, #find-a-co-op-partner,
  #play-testing-feedback (moved in); Voice Channels (unchanged, now last).
- Permissions: Start Here category: @everyone denied Send Messages, Send in Threads, Create Public / Private Threads;
  Owner role allowed Send, threads, Pin, @everyone mentions (+ Attach / Embed on #download-here). #download-here had
  its own overrides denying @everyone nearly everything incl. Read Message History (testers could not have seen the
  download): replaced by the category's read-only rules. #play-testing-feedback denied Send Messages to @everyone:
  now allowed (kept its no @everyone-mention / no TTS denies). Playtesting and Text Channels: open to everyone.
  Every save checked by its API answer (PUT .../permissions 204).
- Posts (docs/release/discord-server-posts.md): #announcements launch post, #how-to-install guide (pinned),
  #features (3 messages), #download-here with both zips of 7a880b02 attached (pinned), #bug-reports guide (pinned),
  #find-a-co-op-partner note. The user's earlier "will be updated when the mod becomes available" note left in place.
- Distribution decision changed: the zip pinned in #download-here (fits Discord's 10 MB with the mod-manager zip);
  the public GitHub release (blocked for me) stays optional.

## GitHub release (user: "I still want everything published to GitHub", 2026-11-03)
- tools/publish-playtest.ps1 first stopped at `gh repo view` (PowerShell 5.1 with ErrorAction Stop turns gh's
  "not found" on stderr into an error); fixed (Continue around that check).
- Published: public repo https://github.com/ashersheets-jpg/crimson-desert-coop-playtest (README with install
  steps, FEATURES.md) and release v0.1.0-alpha-7a880b02 with the player zip and the mod-manager zip (never the
  symbols zip). The project repo stays private.
- Checked: /releases/latest -> the tag (HTTP 200); both files downloaded anonymously are byte-identical to dist\.

## Showcase video and feature PDF (user request 2026-11-03 ~19:00)
- Video: dist/showcase/SeamlessCoopPlus-showcase.mp4 (31.3 s, 76 MB, git-ignored). Recorded with Windows Game Bar
  (Win+Alt+R) in one scripted take (build/port3479/showcase.sh + port3460/showcase_timeline.ps1), cut with Windows'
  own editing API (port3460/cutvideo.ps1, Windows.Media.Editing: trim + join + frame grabs; no ffmpeg here).
  Content: controls card, duel challenge + 3-2-1, Wanderer standing apart, the sprint to Rudbel Fishing Dock, the
  fight in the bandit camp. Raw take kept: dist/showcase/take13-raw.mp4.
- "Lively" teammate (user: not a mirror of us): cdmp_peer gained --name, --record FILE (saves what the game sends,
  with timing) and --mode replay --replay FILE (plays it back as the other player from the game's first state).
  The teammate's part is a separate recorded session (showcase_teammate.sh + teammate_timeline.ps1); both timelines
  start from "the other player is in our world (spawn 1)". Test tool only; the mod is unchanged.
- Findings on the way (12 takes): per-key powershell launches cost ~1-2 s each (one-process timeline instead);
  Game Bar starts recording ~4 s after the hotkey; test saves always load at the bandits start whatever the stored
  position (move_save has no effect on this save); walking routes are unreliable except start -> dock; standing at
  the dock camp >20 s gets you killed.
- The game folder has the released 7a880b02 cdmp.asi again (sha256 076cae39...), shipped ini, saves OK, test save out.
- PDF: docs/release/SeamlessCoopPlus-Features.pdf (2 pages, from docs/release/features-pdf/features.html via Edge
  headless print-to-pdf; images are frames of the showcase video).
- Quest sync (asked in Discord): not implemented: quests stay per save (D42 / NOTES/FLAGS.md F8); the party journal
  notices exist. Possible later: an opt-in offline copy of the host's story progress into the guest's save.
- 2026-11-03: showcase uploaded to the user's Google Drive (My Drive): SeamlessCoopPlus-showcase-1080p.mp4 (80 MB, the original; sent in 9 MB pieces through the Drive page and rejoined there, SHA-256 matched) and SeamlessCoopPlus-showcase-720p.mp4 (9 MB). build/port3460/cutvideo.ps1 got -Kbps (720p re-encode).

## Quest sync (user priority 2026-11-03 ~21:30, asked for by a Discord player)
- Live sync stays impossible: each player's in-process server ignores outside quest requests (D42).
- Offline route investigated: quest progress is one save object, QuestSaveData (stage / mission / quest / quest-gauge
  state lists, last completed main quest; ~1.8 MB, ~50 000 elements). Missions appear in the list as the story
  unlocks them (slot101: 3201 entries / 221 done; slot0: 3475 / 477), so it cannot be an in-place byte patch.
- tools/save/quest_sync.py: transplant(base, donor, object): copies an object between saves, renumbers its elements'
  type indices to the target schema (each save carries its own schema; missing types appended), rebases self
  pointers, splices it in (splice.py) and checks the result (all other objects byte-identical, the moved object
  element-for-element the donor's, encode round trip).
- In game (build/port3479/qsync_test.sh; "Party journal: N missions known, M done" logged at login; test copies of the
  user's saves in slot103 only, removed after; savecheck OK every round):
  * slot0 + slot2's quests (same schema): loads, 2485 known / 456 done. Mechanism works.
  * slot101 + slot0's quests, slot0 + bandit save's quests, bandit save + slot0's character objects, and even the
    40-byte CharacterStatusSaveData alone: all crash at load (exe+0x1563971 -> call 0x0). Identity re-encodes load.
  * Cause: the saves were written by different game versions: the user's slots are the 2026-11-01 backups (older
    build; the game migrates them at load), the bandit save was written by 1.0.0.3479 and its types have more fields
    (FieldNPCSaveData _affinityList, ItemSaveData _averagePrice, QuestSaveData, QuestGaugeStateData...). A block
    written under one field list is unreadable under another.
- Status: works for two saves of the same game version (what co-op partners have); NOT verified for a big story gap,
  because no two same-version saves at different story points exist here and making one needs an in-game save.
  Also open: whether quests alone are enough or the world objects that follow the story (FieldSaveData,
  NPCScheduleStageManagerSaveData, FactionSaveData, KnowledgeSaveData, FogSaveData...) must come along.
- "Continue" picks the slot by the time in its lobby.save: test variants need the bandit save's lobby.save.
- 2026-11-03 ~23:10 FOUND the load crash (exe+0x1563971 -> call 0x0): not the game version. Self pointers come after an
  8-byte run that is FF x 8 everywhere EXCEPT in FactionSpawnStageManagerSaveData, whose elements carry their index
  there (0, 1, 2, ...; 158 in slot101, 170 in slot0). Our tools only knew FF runs, so any change that moved those
  elements (a size change before them: a type added to the schema, a resized object) left their pointers stale, and
  the faction-spawn loader read garbage. Proof chain (all slot103 test copies): one unused type appended -> crash;
  same with a different type, either save -> crash; two types swapped and renumbered (no size change) -> loads;
  header float at 0x08 zeroed -> still crash; plain re-encode -> loads. splice.self_pointers now finds every u32 equal
  to its own position + 4, whatever the 8 bytes before it (slower, ~4 s a save). Also fixed: the blob size stored at
  schema end + 8 was not grown when types were appended.
- RESULT qs7 = slot101 (guest: character at its own spot) + slot0's QuestSaveData (host), QuestGaugeStateData_Stage
  appended: loads, "2486 missions known, 456 done" = exactly the host's totals; position the guest's own. qs6 (type
  appended only) loads with the guest's 2242 / 214. So a story gap of 242 completed missions transfers.
- The earlier "different game versions" verdict was partly this bug: the 40-byte CharacterStatusSaveData move and
  slot101 + slot0 crashed for this reason. Field lists do differ between builds (the bandit save's types have more
  fields), so the tool still refuses saves whose shared types have different fields.
- 2026-11-04 ~00:15 STORY SYNC BUILT AND VERIFIED (D47). common/game_save (C++: decode/encode, schema, object table,
  self pointers, story extract/apply + checks; unit tests: SHA-256/HMAC/ChaCha20 vectors, LZ4, garbage) gives the
  same blob as quest_sync.py for qs7. cdmp_savetool (count/extract/apply/roundtrip/blob) refuses old-format saves
  both ways, damaged files, cut story data, overwriting inputs. Mod: net/story_sync (F8 item "Copy my friend's story
  into my save (guest)", host item "Let my guest copy my story", [coop] share_story, [dev] story_slot), RelKind::Story
  9. cdmp_peer --story ends the game's reliable stream and answers asks. build/port3479/story_e2e.sh (slot103 only):
  ask -> "story ... kept for the next start (492 done; ours 227)" in ~1 s -> restart -> applied in 0.1 s at start,
  backup made -> "Party journal: 2486 missions known, 456 done" at the guest's own spot; ask again + "no" removes the
  pending file. Panel text wraps; the notice waits until the player is in the world (25 s). savecheck OK every run;
  the mod's test backups of slot103 moved to build/test-saves.
- Docs: README STORY SYNC section + Steam Cloud line, FEATURES.md / discord-features.txt (World: Story sync, Works),
  CHANGELOG, PLAYTEST 6c, D47, FLAGS F8 done.
- 2026-11-04 00:15 GAME UPDATED by Steam to 1.0.0.3527 (CrimsonDesert.exe written 00:15:35, PE timestamp 0x6AEA07A6,
  image size 0x13EB2000) in the middle of the final e2e run: phase 1 could not launch, phase 2's mod said "made for
  1.0.0.3479" and stayed off (as designed). The release-candidate zip dd69dd80 (story sync; built, install test 13/13,
  Defender clean, e2e passed on the same binary before the update) is for 3479 only, so it is NOT published: players'
  Steam installs 3527 and the mod would just switch itself off. Next: port to 3527 (docs/post-patch-playbook.md;
  the 3479 exe is in build/exe-archive), then package, publish and update Discord.
- 2026-11-04 ~00:55 PORT TO 1.0.0.3527 (docs/research/port-3527.md). build/port3527/portmap.py maps every address with
  both exes (masked code patterns; globals/vtables through their code references; vtables = RTTI). 40/43 code sites
  one hit; the two action-layer template copies were mapped through their caller (+0x33B0; nearest hit +0x40 was
  wrong). offsets_3527.h generated, 21 files retargeted, horse_diag / load_trace / aggro sites moved, installer and docs
  say 1.0.0.3527; verify-port.ps1 retargeted: ALL OFFLINE CHECKS PASSED. Live: game reader on, 1175 opcodes
  resolved, crash guards installed first try, copy visible, story sync e2e green on 3527 (2486/456 after restart).
  Zone test (bandits save, zone_copy_test): enemy copies loading, suppress decisions correct, no crash after 15 min.
- 2026-11-04 00:54 zone test on 3527 done (build/port3527/regress.sh zone): bandit copies loaded (47 copy lines),
  suppress decisions right, 0 crashes, 0 hook failures; the game's autosave into slot2 restored by savecheck. Running
  duel / revive / rejoin / audit next. User asked for the GitHub link (Reddit): public playtest repo given; the dev
  repo stays private (their call).
- 2026-11-04 01:15 regressions on 3527 (build/port3527/regress.sh, bandits save): duel (rounds, ring-out, scoreboard),
  revive (both ways, up where we fell), rejoin (4 kill/reconnect cycles, spawn per cycle), audit (3 session types,
  each "State audit 30 s after the session: clean"); 0 crashes, 0 hook failures; SAVES OK. Port done.
- 2026-11-04 01:25 RELEASED 0.1.0-alpha+a9c2f5d5 (story sync, game 1.0.0.3527): package (unit tests, install test,
  Defender clean), installed into the game (hash 4DF5FA7A..., smoke start: build A9C2F5D5, reader on, 1175 opcodes,
  no crash, SAVES OK, test save out, shipped ini), GitHub release v0.1.0-alpha-a9c2f5d5 (Latest, public README with
  story sync), Discord updated (docs/release/discord-server-posts.md).

## Co-op saves + live progression (user 2026-11-04 ~01:30: "necessities"; ERSC-style; never overwrite a player's save)
- User: story sync rewriting a player's real save (even with a backup) is unacceptable. Wants: a solo save, a save per
  friend that loads by who you join, back to the solo world on leaving, live quest progress, every save protected.
- FOUND (save_io.cpp, [dev] save_io_log, test save io in slot103): the game READS every slot with CreateFile2
  (save\<id>\slotN\lobby.save + save.save, all slots at the title, again before Continue; caller chain exe+0x15077C6
  <- 0x15064DF <- 0x1506466) and WRITES a slot as a folder swap: MoveFileExW slotN -> slotN_backup, CreateFileW
  slotN_new\<file> (exe+0x1497DF2 <- 0x254B9E8 <- 0x254A93E), MoveFileExW slotN_new -> slotN, slotN_backup ->
  slotN_backup_<n>_delete, DeleteFileW its files (once per file: lobby.save then save.save). All through kernelbase:
  a path redirect at that layer can give co-op play its own save folder (ERSC .co2 style) and keep solo saves untouched.
- Live quest progress idea: quests advance on client requests the in-process server accepts (MissionTriggerInOutReq,
  QuestDialogEndReq, QuestDialogInterrogationAnswerCompleteReq, StartDiscoverLevelSceneObjectReq ...), not the
  cheat ones it ignores: mirror the host's to the guest.
- 2026-11-04 01:45 HOTFIX (user approved): story sync never writes a player's save (story_sync.cpp kWritesPlayerSaves =
  false): asking says it returns with co-op saves; a story the previous build kept is discarded at start, with a notice.
  Verified (story_e2e + a planted pending file): "kept story ... discarded", st1 still 3242/227, folder empty. Docs:
  README section "CO-OP SAVES AND SHARED STORY (coming next)", FEATURES/discord-features "Coming next", CHANGELOG.
  User clarifications: the HOST also gets a save per friend (solo run untouched); quest log/progress must update in
  real time for all players (no relog).
- 2026-11-04 ~01:55 hotfix 2bd32050 RELEASED: package (tests, install test, Defender clean), installed in the game
  (smoke start OK, SAVES OK), GitHub release v0.1.0-alpha-2bd32050 (Latest), Discord updated
  (docs/release/discord-server-posts.md).
- 2026-11-04 02:17 CO-OP WORLDS, SAVE SEPARATION WORKS (save_io.cpp, [dev] coop_world=<name>): kernelbase hooks
  (CreateFile2/W, MoveFileExW, ReplaceFileW, DeleteFileW, Create/RemoveDirectoryW, GetFileAttributes(Ex)W,
  FindFirstFile(Ex)W, CopyFileW/ExW/2, SetFileAttributesW) send every path under save\<account>\ to
  Documents\CrimsonDesert-CoopPlus\Worlds\<world>\<account>\; a new world = a copy of the player's slots (made under
  .making, renamed when complete). Test (world_test.sh, in-game save into SAVE 4): the save went into the world
  (world slot103 new hash 9ccf2ad4..), the REAL slot103 byte-identical (4255263760097ad4, same mtime), all real saves
  = backup. First try failed safely: the game also COPIES files during a save ("CopyFileW Error, Error Code : 3",
  then "Commit Save 실패 [slot103]" and its own rollback); copy calls are hooked now.
- In-game Load path (3527): Esc > Others > Load > slot > Space > hold Space; Continue at the title calls
  exe+0x19B09D0(manager=[[exe+0x71BE3F8]+0x48], out, slot number, ..) via exe+0x2EBB220; the in-game Load goes
  through exe+0x2EE1D90 -> 0x19B09D0. Each load sends GameLoadingStartReq (the sniffer sees it).
- Live quest research: CompleteQuestByCheatReq's Troc handler (vtable 0x5EE9578 slot 2 = exe+0x2CD4180) only parses
  and queues (the generic [this+0x21] check is in every handler); the gate is further in: needs live tracing.
- 2026-11-04 ~02:45 coop_worlds.cpp ([coop] coop_worlds=1, off by default): world per partner (with-steam-<id64>,
  host and guest); session up 8 s -> 10 s notice -> Esc > Others (804,50) > Title Screen (466,724) > hold Space ->
  at the title set_world -> Play (E). First live run: menu -> title -> world made (7 slots copied) OK, but E at +8 s
  was ignored (title not ready): now E is pressed until the sniffer sees GameLoadingStartReq (every 12 s, 8 tries).
  Partner gone 45 s -> back to own saves the same way. Guest: story_auto_request() on connect keeps the host's story
  in memory; at the title it goes into the WORLD's newest slot only (story_put_into_world). docs/design/coop-worlds.md.
- 2026-11-04 ~03:05 worlds tests 2-3: INTO the world works end to end (title, world made, the host's story put into the
  WORLD copy only "492 missions done", Play, loaded). OUT failed: a tutorial popup ("149. Lightning", from the host's
  story's new skills) blocks Esc; quick Space taps do not close it (the test now holds Space as a player would).
  Fixes: retry with a 30 s backoff, 5 tries then a message with the manual way; at the title (out of the world 10 s,
  not loading) the saves switch at once (manual fallback / a game started while a world is wanted); the journal
  starts a new baseline at every load (it had reported the host's 240 completions as ours; now 0). Worlds moved to
  %LOCALAPPDATA%\CrimsonDesert-CoopPlus\Worlds (Documents is OneDrive-synced here; a sync lock mid-swap could fail a
  save). Old test worlds moved out of Documents to build/test-saves/old-test-worlds.
- 2026-11-04 03:10 CO-OP WORLDS FULL CYCLE GREEN (worlds_test.sh run 4, one PC, echo partner with --story m0):
  own world (st1 = m101, journal 2242/214) -> partner connects -> 10 s -> title -> world with-test-echo made (7 slots
  copied) + the host's story put into the WORLD copy -> Play -> "Party journal: 2486 missions known, 456 done" (the
  host's) -> partner gone 45 s -> first try blocked by a tutorial popup (retry 30 s) -> second try: title -> own saves
  -> Play -> 2274/214 (own story). Journal: 0 false completions. Real slot103 byte-identical (c3fb54d4), SAVES OK.
- 2026-11-04 03:25 live quest research: CompleteQuestByCheatReq (3041) sent 12 times (dev script, echo session, recorded:
  the 17-byte packets reach the server) with mission key 0xF42BC and quest keys 0xF42DF / 0xF456C in 4 layouts each:
  no CompleteQuestByCheatAck, no mission/quest list change. Ignored on retail (KillCheatReq still works: the quest
  cheat has its own gate, behind the queued dispatch). Next route: the client DRIVES much of the quest/stage system
  (one session: StageBranchStateReq 9, SetQuestDialogReq 188, GamePlayTriggerInOutReq 195, UpdateStageChartEventReq
  32, SequencerTriggerVolumeInOutReq 4, UpdateSequencerGamePlayDataEventReq 23); mirroring the host's to the guest's
  server (both worlds start from the same QuestSaveData now) is the live-sync candidate. Proof needs a recording of a
  real quest step: record, then replay only those requests on a fresh load and see the step complete.
- 2026-11-04 08:55 LIVE QUEST: MIRRORING WORKS (first proof). build/port3527/live/transitions.py scans recordings for
  mission state changes (UpdateMissionListAck 2360, walked like party_quests.cpp) with the client requests before
  each: the 3479 archive has "Head to Hernand" (FFFFF4D6) 2 -> 5 right after ONE MissionTriggerInOutReq
  (2899: u32 mission, u32 user key, u8; recorded 2899 d6f4ffff 010010a0 00, answered by UpdateTraceMissionAck 2459
  pointing at the next step FFFFF4D5). Replayed on 3527 (mission_test.sh, coop-test-bandits, nobody moves): with
  the real user key A0100001 the server answered 2459 and moved "Head to Hernand" 2 -> 5. With a wrong user key
  (our {U} placeholder gave 90100000) nothing. The user key is in every 2360 at offset 6. The mission finishing made
  the server AUTOSAVE into slot2 (known, savecheck restored it from the backup; in a co-op world it goes to the
  world). Next: does 2899 finish steps of other kinds (talk / defeat / interact / read)?
- 2026-11-04 09:00 2899 for steps of other kinds (talk "Blue Fang", defeat "White Bearclaw", interact "bell in
  Varnia", read "Ursula's journal") and for the NEXT step (not started) did nothing: the server checks the step's
  kind and order. The quest cheat is dead for good: its parser (exe+0x2CD4180; u8 x3 + 14-byte entries u64/u32/u16)
  calls no handler at all on retail (8 more layouts with the user key: nothing). build/port3527/troc_fields.py reads
  any request's field sizes from its parser: QuestDialogEndReq = 30 bytes (u32 NPC, u32 dialog, u32 x3, u8 x2, u32 x2).
- 2026-11-04 09:13 LIVE QUESTS END TO END (one PC): new src/mod/net/quest_live.* (RelKind::Quest = 10, [coop]
  live_quests=1, only while both players are in a co-op world; [dev] live_quests_any_world for tests). Test A
  (live_test.sh, peer --quest): the partner's 2899 for "Head to Hernand" with a FOREIGN user key arrived, the mod
  wrote ours (A0100001), injected it: "Head to Hernand" 2 -> 5 + "Quest step done together" notice; not echoed back.
  Dialog ends are relayed with an NPC mapping from SetQuestDialogAck 3086 (unproven: no talk-step sample yet).
- 2026-11-04 09:21 live quests SENDING side green (qliveC): our client's 2899 for "Head to Hernand" -> "Live quests:
  sending our trigger" -> the peer received 01 01 53 0b d6f4ffff 010010a0 00. Both directions now shown on one PC
  (A: receive + inject + step done; C: observe + send). A plain teleport to the Hernand RPS table did not make 3527's
  client send the trigger (qliveB: no 2899 at all; the zone or a condition differs), so C sent it by dev script
  (the mod sees it exactly as the game's own). Next: a real dialog sample (talk_test.sh: greet the Hernand boy).
- 2026-11-04 09:25 talk sample attempt (talk_test.sh, Hernand boy): Greet = ambient small talk, only
  SetQuestDialogReq/Ack (the boy's dialog key went 0C0B -> 0C0A after the greet), no QuestDialogEndReq. Other NPCs
  near there are townsfolk (farmer, fishermen). No quest NPC reachable by script in the test saves -> F14 narrowed
  to one talk step + one defeat step. Game state after: release asi a51453d2, shipped ini, SAVES OK, test save out.
- 2026-11-04 09:57 defeat steps, one PC: new dev script command `killall` (KillCheatReq on every loaded actor but
  wild animals). defeat1: teleport to Riverward Outpost (4 "Defeat the enemies around ... Posthouse" steps under way),
  74 kills (deaths confirmed, NotifyHandleDeadAck): no mission change (the actors there were mostly Hernand soldiers
  and townsfolk). defeat2: the bandit camp near the start (~10 bandits, -10310 -3675), 77 kills: no mission, quest or
  faction message at all (only an autosave request, restored). Either cheat kills do not count for quests (as for
  challenges, BLOCKED T10) or these camps are not the steps' camps. build/port3527/live/defeat_targets.py maps under-way
  defeat steps to places (all_world_locations.json). Defeat steps stay unproven; a real recording (F14) settles it.
- 2026-11-04 12:30 Roddy's idea (playtest Discord: find quest progress in RAM by diffing memory around a finished
  step) followed. New dev tools: [dev] quest_events_log=1 (src/mod/net/quest_events.*: logs every game event posted to
  the quest system, exe+0x19434D0(handler, pa::GameEventHandlerParameter*); MissionTriggerInOutReq's handler only posts
  one, type 0x11/0x12 + mission + user) and dev script memscan / memdiff / memwatch (src/mod/net/dev_memscan.*; dev_watch
  now watches up to four addresses and passes the copy protection's own single-step exceptions through).
  - qev1: 40 cheat kills posted NO game event at all -> why cheat kills never count for quests or challenges.
    Recordings show hits and deaths are worked out by the game's in-process server (AttackerAck / NotifyHandleDeadAck
    from the server; the client never sends a kill), so a real kill cannot be faked with a request either.
  - msc1: 101 copies of "Head to Hernand"'s key in RAM; after the proven trigger 3 had their byte at key+4 go 2 -> 5.
  - mw2 (hardware watch on 4 copies): both real writes are exe+0x1C119E7 `mov [rdi+0x2C], al` (rax 5, old 2) inside
    a record copy exe+0x1C119C0, called by exe+0x1C37B10 = upsert into the mission table (record: u32 key +0x28,
    u8 state +0x2C, u8 +0x2D). One write on the server thread, one on the client's copy.
  - Server chain: exe+0x2B9AAD0 -> 0x2B95290 -> 0x2B95BE0 -> 0x1C41B00 -> 0x1C41920 -> 0x1C37B10. exe+0x2B95BE0
    (manager, u32* result, u16* mission INDEX, u64* time, u8) is the quest advancement routine: it queues state
    changes with exe+0x1C3F0F0(manager, u32* mission key, ctx, u8 state 2/5, changes*) and commits them with
    exe+0x1C41B00. Seven callers. quest_events now also logs both (ADVANCE / set mission) for the next test.
  - Next: one test logging ADVANCE / set mission around the trigger (manager pointer, index, time), then call
    exe+0x2B95BE0 on the server thread for a defeat step = Roddy's "sync the stage to the host" through the game's
    own routine (runs the next step and the rest, unlike a raw state poke).
- 2026-11-04 12:57 STOP on tests: Steam Cloud. Steam uploads the save slots the moment the game exits (cloud_log:
  "AutoCloud on exit", 12:46:50 slot2 upload OK), seconds BEFORE savecheck restores slot2. So the test autosaves have
  been reaching the user's Steam Cloud copy of slot2 (local slot2 is right: SAVES OK). The 12:50 upload hung
  (googleapis "failed to issue"), and Steam now sits at "Synchronizing cloud" and launches nothing. Fixed in the
  kit: events/mission/live/talk_test.sh run with [dev] coop_world=devtest; events_test.sh builds that world itself
  (copies of the slots + the test save as slot103) so a test never writes the real save folder. Flag F15.
- 2026-11-04 13:36 Steam restarted (user OK): the hung upload cleared and Steam uploaded the REAL local slot2
  (989842 bytes, not the 987757-byte test autosave), so the cloud copy is right again. Since then no test has
  uploaded a slot (tests run in the scratch world devtest; cloud_log shows no "Need to upload"). F15 resolved.
- 2026-11-04 13:48-14:12 the quest advancement routine, called by the mod (dev script `advance <key>`; runs inside
  MissionTriggerInOutReq's handler on the quest server's thread, woken by a mission-0 trigger):
  - adv1/2: the proven trigger -> ADVANCE mission index 6597 from exe+0x2B95764, thread = the quest server's.
    The mission table (exe+0x71C1940: +8 count, +0x58 info* per index) has the key at info +0x08.
  - adv4: "Defeat the Helms at Batihar" and "Defeat the enemies around Deepwoods Beacon" 2 -> 5 from far away, with
    the consequences: UpdateQuestGaugeAck, DeleteFactionSpawnStageAck (the camp counts as cleared), experience
    (AddSubLevelExperienceAck).
  - adv5 (time 0): talk steps "Talk to the injured Blue Fang", "Talk to the chief of Tinkerton" 2 -> 5; a step
    already done is refused (error result, no change); a step NOT STARTED ("Visit all Greymane Shrines") goes
    0 -> 2 -> 5, so callers must only advance steps under way.
  - Built into live quests: src/mod/net/quest_advance.* (hooks: state setter for the manager, trigger handler to run
    requests; site bytes checked; installed in late init when [coop] live_quests is on). quest_live sends "step
    finished" (opcode 2360 + u32 key) when one of our steps goes 2 -> 5 (not while a save loads, not the partner's own
    step coming back); the receiver advances it only if it is under way there (waits 60 s for it to start).
  - lstep2 (peer simulator): ours -> partner `01 01 38 09 d6f4ffff`; partner's "Defeat the Helms at Batihar" ->
    advanced here 1.6 s later -> "finished here too" toast. lstep3: a partner's step not started here is dropped
    ("not under way here"), no change. Two-PC test still to come. Game state after: release asi a51453d2, shipped
    ini, SAVES OK.
- 2026-11-04 14:30-15:00 RELEASED 0.1.0-alpha+16df564d, co-op saves + live quests as an EXPERIMENTAL OPT-IN (user:
  "make the option to opt in very obvious and easy to turn on ... when they first launch the game"). src/mod/optin.*:
  a card on the title screen at the first launch of this build (also for players updating: [general]
  experimental_asked), Shift+F5 / View+D-pad Up = turn on (applies at once: save hooks, co-op worlds, quest server
  hooks), Shift+F6 / View+D-pad Down = not now; F8 > Settings first item switches it ([coop] coop_worlds, off by
  default; off = none of those hooks). Tested in game: card at the title and in the world (screenshots optin1_card,
  smoke16), turn on (log "turned ON", ini coop_worlds=1, hooks installed), not now (stays off, no hooks). Package
  (unit tests, install test all PASS, Defender clean both zips), installed into the game (asi aadcb83c..., player's
  ini kept; build/port3460/cdmp.ini.orig = it; old one kept as cdmp.ini.orig-2bd32050), smoke start OK, SAVES OK.
  GitHub release v0.1.0-alpha-16df564d (Latest, README/FEATURES refreshed). Discord: docs/release/discord-server-posts.md.
  Test scripts in build/port3527 now restore this release's asi.
- 2026-11-04 15:10-15:50 Reddit questions tested on one PC (build/port3527/session_test.sh, focus_test.sh):
  - Far apart: peer --offset 15000 (new peer option --far START,END,M moves the echoed partner away and back). BUG
    found: a partner 15 km away was drawn frozen next to us (frame "2 m": the game cannot place an actor on ground it
    has not loaded) and the T06 drift resync reloaded it every 15-20 s. FIX (coop.cpp): beyond 600 m the partner's
    character is removed, within 400 m it is loaded again where they are; the drift resync only runs while near.
    far5: "15000 m away" -> character hidden, frame shows 15000 m, our own character untouched (shot far5_a); back at
    2 m -> loaded again (far5). Connection steady (~20 ms, no loss), 1 harmless warning. Not released yet.
  - Focus (spirit slow motion; pad L3+R3, PC C+RMB per guides): could not trigger it by automation. C+RMB swung the
    weapon (an attack on this setup), X did nothing, a dev pad press does not reach the game without a real pad
    (that experiment reverted). The mod does not sync game speed, so Focus is local to the player who uses it.
- 2026-11-04 16:19 Focus tested by hand (user's PlayStation pad; the game was on monitor 2, captured from x 1920):
  focusman session (relay partner beside us), Focus on from ~16:19:20 for 35+ s (green tint, particles, "Cancel").
  During it: ~60 fps steady (overlay frames ~300 per 5 s), link rtt 12-16 ms, ~1.9 kB/s, clock offset steady, the
  partner's character drawn and never reloaded, no warnings. A dev pad press does not reach the game (Steam Input
  pad), so automated Focus tests are not possible here. Two-PC view (the other player's screen) still untested.
- 2026-11-04 16:25-16:48 Focus, the other way round (user's question: does the FRIEND's Focus slow my world?):
  recorded a session with the user using Focus (peer --record), replayed it as the stand-in (peer --mode replay;
  the stand-in arrives only after the recording's title minutes). Stand-in in our world 16:40:45 -> 16:48, Focus
  included: our screen never showed the Focus tint (269 frames every 1.5 s, build/port3460/capture_game.ps1), link
  steady, no resync. Limits: the stand-in was off camera (its animation not seen) and the game's time scale was
  not measured. So far: Focus is local to the player who uses it (it sends nothing to the game's server either).
  CORRECTION: the 16:25:53 game exit (first thought a crash from the user's lock-on press) and the 16:37:00 one were
  my own test scripts: stopping a background test (TaskStop) left its inner bash alive, which closed the game on its
  schedule (taskkill by name). Fixed in the kit: each test script closes only the game PID it started.
- 2026-11-04 17:14-17:38 Focus during a fight, one PC: NOT achieved. As host the game cannot open its UDP port next
  to the relay peer; a guest recording with [dev] zone_copy_test (the guest sends its zone enemies) replayed as the
  host gave copies 63 s out of step that loaded and were removed within ~30 s (plus character-load errors), and the
  user's character never moved (no fight). A faithful test needs two worlds = two PCs: added as PLAYTEST-1 §6d
  (guest Focus mid-fight, host Focus, far apart and back). Game state after: release asi, player ini, SAVES OK.
- 2026-11-04 18:05-18:20 RELEASED hotfix 0.1.0-alpha+cea405cf (far-away friend hidden beyond 600 m, back within
  400 m; user approved): package (unit tests, install test all PASS, Defender clean both zips), installed in the
  game (asi b4d50b37..., player's ini kept), smoke start (build CEA405CF, no errors, SAVES OK), GitHub release
  v0.1.0-alpha-cea405cf (Latest), Discord (docs/release/discord-server-posts.md). Test kit restores this asi now.
- 2026-11-04 19:00-20:00 Blocking slow motion in co-op (playtester poll: remove ability slow-mo for sync), step 1:
  find the game speed. New dev tools: dev script fscan / ffilter / fwatch (float narrowing, dev_memscan.*) and
  waitfile (a test driver signals steps); build/port3460/tintwatch.ps1 spots the Focus tint on the game window and
  drives the scan, so the user only presses Focus (X on keyboard / L3+R3; a simulated X press does not start it).
  Rounds fscan3 / fscan4 (user's Focus x3 each): 7 M floats of 1.0 -> 24-47 that dipped every Focus and came back,
  but while Focus was on they held unrelated values (0.96, 0.6, 0.2, 0.067 ...; a watch on four: exe+0x495C4E9 /
  0x495B542, per-frame animation blend history). No single global speed float. The packet recording shows no
  request tied to Focus (client-side). Next: watch the writer of values that sit at one designed factor during
  Focus (e.g. the 0.6 group) to find the code that applies the slow-down, then skip it in co-op.
  ALSO: savecheck --fix had restored the game's settings files in save\ (it reverted the user's new keybind preset;
  their files were put back from the kept copies); savecheck now compares only the save slots (depth >= 2).
- 2026-11-04 20:35-21:21 NO SLOW MOTION IN CO-OP, done and verified on one PC (user watching). Focus is a server
  "special mode": EnableSpecialModeAck (3126: u32 user | u32 mode 101 | f32 FLT_MAX | u32 | u32) on, DisableSpecialModeAck
  (2701) off; the client never asks. src/mod/net/slowmo.* drops the switch-on in the sniffer's receive filter while a
  co-op session is up ([coop] block_slowmo=1 default, F8 "Slow motion (Focus) in co-op"; [dev] block_slowmo_always).
  Found via: game names (ClientFrameEventTimeScale / TimeScalePreset _globalTimeScale _playerTimeScale; that event's
  handler exe+0x7A2C30 never fired for Focus), float and double scans of the speed (no global speed value), then the
  recordings (3126/2701 line up with the tint exactly). Automatic Focus: keybd_event X with scan code 0x2D, retried
  until the tint shows (build/port3460/focusprobe.ps1 -KeepS; tintwatch.ps1; the first press after a load often fails).
  Results: control run = normal Focus (user saw the tint); block run = Focus triggers, no tint, no slow motion (user
  watched); Spirit still refills: [dev] stat_log=1 shows stat 17 (Spirit, max 30000) 15915 -> 30000 within 5 s of
  Focus with the block on (after three Force Palms spent it). Two-PC test pending (PLAYTEST-1 §6d). Not released.
- 2026-11-04 21:22-21:40 RELEASED 0.1.0-alpha+69b9b13e (no slow motion in co-op; user approved): package (unit tests,
  install test all PASS, Defender clean both zips, shipped ini block_slowmo=1), installed in the game (asi 33f2cf0d...,
  player's ini kept), smoke start (build 69B9B13E, "Focus's slow motion is skipped in co-op sessions", SAVES OK),
  GitHub release v0.1.0-alpha-69b9b13e (Latest; the publish script's release step fails on quotes in -Notes, so
  gh release create --notes-file), Discord (docs/release/discord-server-posts.md). Test kit restores this asi now.
- 2026-11-04 22:20-23:25 PORT to game 1.0.0.3549 (Steam update; user: "make sure everything works for the newest
  update"). Details: docs/research/port-3549.md. Every mod address maps to itself (map_all.py, all +0, heads equal);
  offsets_3549.h = 3527's table with the new PE stamp / image size; installer, README and release texts say 3549;
  verify-port.ps1 follows the header include chain (all pass). AI battle-type jmp head re-read (protection moved).
  Live, one PC, devtest world, SAVES OK each run: session + partner (p49b/c), aggro hook, Focus block (3126/101
  unchanged, fb49/fb49b), live quests both ways (ls49c), far apart (far49), opt-in card (opt49).
  Found + fixed: (1) loads ~50 s slower since the slow-motion block (VirtualQuery per received packet; in-world ->
  LoadingCompleteAck 37-63 s since 20:58 vs 12 s before; the released 69b9b13e has it) -> direct read, 12 s again;
  (2) a transient crash-guard hook failure (1.3, trampoline out of range; retry succeeds) left the horse copy off
  for the session -> mount fix installed after the guard tries; (3) hook failures log the inner reason.
  Not released yet (needs the user's OK). Game state after: shipped ini, release asi 69b9b13e (off on 3549), no game
  running.
- 2026-11-04 23:20-23:40 RELEASED 0.1.0-alpha+0364c3f0 for game 1.0.0.3549 (user: "Yes, release it"): package (unit
  tests), install test all PASS, Defender clean both zips, installed in the game (asi 1d5bee23..., player's ini kept,
  installer accepted 3549), smoke start (build 0364C3F0 on 1.0.0.3549, game reader on, 1175 opcodes, SAVES OK).
  GitHub v0.1.0-alpha-0364c3f0 (Latest; README/FEATURES pushed by hand: publish-playtest.ps1 now stops at git's
  LF/CRLF warning under ErrorActionPreference Stop). Discord: docs/release/discord-server-posts.md. Test kit
  (build/port3527/*.sh) restores this asi now.
- 2026-11-05 00:00 Test save slot103 removed from the user's save folder AND Steam Cloud. It had been uploaded to
  the cloud by early tests (before the devtest world) and Steam re-downloaded it at every Steam launch; the release
  smoke start + savecheck --fix (which touches slot103 to be newest) left it as the newest save, so the title's Play
  would have loaded the test world. build/port3460/cloud_remove_slot103.ps1: game to title, delete slot103 while it
  runs (copy in build/test-saves/slot103-cloud-copy-2026-11-04), exit -> Steam synced the deletion; a second launch
  did not bring it back. SAVES OK, user's ini and release asi in place.
- 2026-11-05 00:25-00:45 MISTAKE + FIX: a New Game probe (build/port3527/newgame_probe.sh, empty co-op world
  "newgame-probe": the title shows Play/Load greyed, only New Game; a short Z press did not start it) ran right after
  the user had played (their slot2 saved 00:16:48, real progress). trial.ps1 deleted the user's session log
  (cdmp.log) and rewrote cdmp.ini from the 2026-11-04 14:40 copy (the user's opt-in answer is gone: the card asks
  again). savecheck.sh compared with the 2026-11-01 backup and its --fix would have rolled slot2 back at the next
  test (it was not run). Fixed in the test kit (build/, not in git): savecheck.sh --snapshot (player's slots + ini
  at test start; compare / --fix / --restore-ini against that), test_guard.sh sourced by every game test script
  (refuses to run while Crimson Desert is running; snapshots first), run_echo.ps1 / trial.ps1 never close a running
  game, trial.ps1 keeps the last log and builds the test ini from the snapshot. Reference now
  SaveBackups/reference-2026-11-05_00-30-00 (= the user's real slot2 of 00:16:48), SAVES OK.
- User request (00:20): on the first invite, choose "new save" (both start the game from the beginning) or "start
  from the host's progress"; solo saves must stay safe. Not built yet (FLAGS F17); a no-code way for tonight given.
- 2026-11-05 00:30-01:25 F17 "new save or the host's progress" on the first session: BUILT, NOT RELEASED.
  coop_worlds: the host gets a card (Shift+F5 new save / Shift+F6 the host's progress), the guest's game follows the
  host's choice (RelKind::World = 11, peer sim --quest 0101 --rel-kind 11). A new-save world is a COPY of the
  player's slots (an EMPTY folder left the title holding its old slot list: "failed to load save", no buttons) with
  coopplus-world.ini start=new; at the title the mod holds Z (New Game; a short press is ignored), the player
  accepts the agreement and picks the difficulty; new_game_started=1 once in the world (later sessions: Play).
  Guest test nw3/nw4/nw5: world made, New Game, new game running in the world, player's saves untouched (SAVES OK).
  BLOCKER: the partner's character copy crashes the game seconds after it loads in a brand-new game: its action
  layer has a chart but never an action (the opening character), and three different update functions read the
  missing action (0x251A1AD via CC tick, then 0x96918B via 0x9671E6). The CC tick guard now covers the player copy
  too (horse_diag.cpp) and a co-op world switch drops the partner's old character (coop.cpp; it was loaded ~1 km
  off in the new world and swapped a moment later), but the copy itself still crashes. Next: why a fresh
  character's copy gets no action (its spawn template is 10593 bytes against 12610 in a normal save).
  Also fixed: the co-op world's background story ask no longer shows "Story copy: not done" (user saw it on every
  test); peer sim answers "no" without --story; [dev] coop_world is now the tests' "own saves" (set_dev_home) so a
  switch can never reach the player's real folder (test nw1 had loaded the user's own save: nothing saved, SAVES
  OK); [dev] coop_world_as_host; worlds_configure keeps a dev test partner.
- 2026-11-05 02:15 FIRST TWO-PC PLAYTEST (friend Matikjr hosted, the user joined; 42 min): docs/tests/playtest-2026-11-05.md.
  Worked: Steam invite/join, connection (rtt p50 43 ms), hits on NPCs for both, co-op saves made for both (solo saves
  untouched). Broken: the host's game switching to the title by itself felt like a virus; revive; the host's NPCs
  missing in the first town; the host's UI gone after dying + checkpoint respawn (and no horse prompt); duel hits;
  the guest's co-op save made 1 s before the host's story arrived (different town states); challenges never end;
  laggy movement both ways (not the network). Guest log kept in build/playtest-2026-11-05/; host log asked for.
- 2026-11-05 02:40 Playtest fixes, not released: (1) the co-op save switch only on the player's answer (card: go now
  / I'll do it; banner while the mod drives the game; no keys at all when the player goes to the title); (2) a
  friend's revive is ignored while our own respawn is under way. The host's log is lost (friend uninstalled):
  the death/respawn/UI bug gets a one-PC repro next. Order agreed with the user: 1 switch, 2 death/revive, 3 lag +
  duel, 4 a fresh save for both on the first session (replaces "wait for the host's story"), 5 challenges end.
- 2026-11-05 02:35-04:30 (user asleep, autonomous) Playtest #2 FIXED: the lost HUD / pause menu / Ride prompt after a
  co-op death. Reproduced on one PC (death_test.sh: KillCheatReq on our own player, the mod's revive): EVERY co-op
  revive (checkpoint U and instant Q) left the HUD gone and the menus locked. Cause: our own NotifyHandleDeadAck
  (2985) puts the client into its death state; only the game's death screen (UIGamePlayControlReviveModal,
  vtable 0x5A13298, factory 0x10873F0) ends it, and that screen never opens once a copy was loaded (old finding).
  .data diffs (new dev gsnap/gdiff, ptrscan, watchstatus, key) narrowed it to client UI state, not a global flag.
  Fix (revive_prompt.cpp revive_filter_s2c): once a copy was ever loaded, our own death notice is not delivered to
  the client; the server still has us dead, so the mod's prompt, the friend's revive and the server revive work,
  and the client keeps its HUD and menus (dt9/dt11/dt12: HUD through the death, Esc opens the menu after). Also:
  the prompt now shows whenever a copy was ever loaded (a death after the friend left had no screen at all), the
  death screen replica looks like the game's (grey veil, dark red rim, no red bar). Dead end noted: re-addressing
  the notice to our body actor crashes the game (dt10).
- 2026-11-05 04:35 Playtest #3 (lag + duel) FIXED on one PC: (a) the duel's own HP came only from a VaryHPAck, so an
  unhurt player had none and EVERY duel hit on them was dropped ("our hp -0.0" in the playtest): seeded from the
  game's own reading (player_hits hits_note_own_health); (b) swings judged against the drawn copy (late) OR the
  partner's latest reported position, reach 3.5 m / arc 75 deg; (c) playout delay 300 -> 150 ms (the copy was drawn
  ~400 ms late; network loop p95 2 ms, rtt 43 ms in the playtest), migrated once in existing inis (delay_tuned).
  duel_test2.sh (scratch world): our hp 160 at the start, 9/9 swings HIT, 9 echoed hits applied, round lost "down".
