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
