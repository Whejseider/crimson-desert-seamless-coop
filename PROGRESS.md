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
