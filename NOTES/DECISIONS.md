# Decisions

## 2026-10-31 D1: Project location (superseded by D13)
- Options: (a) new standalone repo, (b) `CrimsonDesert-SeamlessCoop/` folder inside `ashersheets-jpg/elden-force-source` on branch `claude/seamless-coop-mod-t4lqdd`.
- Chosen: (b) for now. This session runs in a cloud container whose GitHub access is scoped to that repo and branch. Nothing in the Elden Force project is touched (only a new top-level folder and a new, path-filtered workflow `.github/workflows/cdcoop.yml`).
- Revisit: moving it to its own repo (history can be split with `git subtree split`) before a public release. Suggested to the user.

## 2026-10-31 D2: Toolchain
- Dev box (Linux cloud): CMake 3.28 + Ninja, GCC 13 for native unit tests, MinGW-w64 13 (posix threads) for the Windows DLL cross-build.
- Release/CI: MSVC on GitHub `windows-latest` (static CRT, so no VC++ redistributable is needed). Same sources build on both.
- Why: the brief asks for MSVC/CMake; no Windows machine is available in this session, so MinGW proves the DLL compiles and CI makes the MSVC build.

## 2026-10-31 D3: Steam access without the SDK at build time
- Options: link `steam_api64.lib` from the Steamworks SDK vs. resolve the flat C API from the game's own `steam_api64.dll` at runtime (GetProcAddress).
- Chosen: runtime resolution. No SDK files in the repo, no version lock to the SDK the game shipped with, and if an export is missing co-op stays off instead of the game failing to start. We never call `SteamAPI_Init` (the game owns it).

## 2026-10-31 D4: Loader
- Ship the mod as `CrimsonDesertCoop.asi` for Ultimate ASI Loader (ThirteenAG), which the Nexus survey found is how existing CD DLL mods load (ASI in `bin64`, loader as winmm/dinput8/version.dll). We don't ship a proxy DLL of our own, so we coexist with other ASI mods. To be verified in Phase B (M0).

## 2026-10-31 D5: Config format and library
- TOML, parsed with toml++ v3.4.0 (MIT, vendored single header in `third_party/`), exceptions off. Bad values log an error and keep the default; a config typo never stops the game.

## 2026-10-31 D6: Data-driven M0 probe
- The probe reads signatures and pointer chains from `config/probe.toml`, so offsets found in Phase B can be tried without rebuilding. Reads use ReadProcessMemory on our own process, so a wrong pointer logs `<unreadable>` instead of crashing.

## 2026-10-31 D7: Default scaling curve
- Boss health 1.0/1.7/2.3/2.8 (the brief's starting curve). Regular enemies slightly gentler (1.0/1.5/2.0/2.4), elites in between. Damage scales mildly (up to x1.3-1.4). All in `config/scaling.toml`, to be tuned in play tests.

## 2026-10-31 D8: Guest saves
- Default `guest_save_mode = "separate_slot"`: guests play a co-op copy of their character and their single-player save is never written. Backups before every session are on by default. (Brief 6.3.)

## 2026-10-31 D9: Comfort Pack = our own implementation of the ideas
- Other authors' data patches are mostly "ask first" (unverified) and would have to match exactly on both PCs. We reimplement the ideas as host-synced values in our DLL, credit the mods that inspired them, and list compatible mods as companions. See `docs/mod-permissions.md`.

## 2026-10-31 D10: Build on cdmp / cd-coop-3446 (user decision)
- Options: build on the GPL-3 code (fast; project becomes GPL-3) / clean-room MIT (slow) / contact authors first.
- User chose: build on it. Imported nouzert1/cd-coop-3446 at b7ae133 unchanged into `upstream/` (minus a bundled
  binary), project LICENSE = GPL-3.0, CREDITS.md added. Upstream kept near-pristine (patches listed in
  `upstream/README.md`) so we can keep merging its fixes; it was updated today.
- Plan: `docs/integration-plan.md`.

## 2026-10-31 D11: Phase B runs on the user's PC attached to this session (user decision)
- The user will attach their gaming PC through Remote Control. No folders were shared yet at the time of asking.
- Still gated by "GO: game testing is clear" for launching the game / screen control. Shell and file work on the PC are
  allowed before GO (brief section 0 permits files and shell), but nothing is installed into the game folder before GO.

## 2026-10-31 D12: Upstream portability patch
- `EnemyZoneMachine(Settings s = {})` fails on GCC/Clang. Split into two constructors
  (`upstream/patches/0001-...`) so upstream's unit tests run on Linux and in CI. No behaviour change.

## 2026-10-31 D13: Own repository, fully separate from Elden Force (user instruction)
- The user wants Co-op+ entirely separate from the Elden Force repo and from any Elden Ring work.
- Moved the project to its own repository `crimson-desert-seamless-coop` with its history (git subtree split of the
  four Co-op+ commits; no Elden Force commits or files in it). CI workflow moved to `.github/workflows/build.yml`.
- The Co-op+ commits were never pushed to elden-force-source (push was refused), and the local Elden Force branch was
  reset to its original commit, so the Elden Force repo is untouched on GitHub and locally.
- The integration cannot create repositories (403), so the user creates the empty repo; then we push `main`.

## 2026-10-31 D14: Remote Control attach is not possible (Windows PC) -> proposed: local Claude Code session on the PC
- list_computer_folders: computer DESKTOP-SM0R9J3 offers one folder, `JumpRing` (the Elden Ring project; never to be
  used for Co-op+), under "cannot_be_used": cloud sessions can only attach folders on Mac/Linux computers; this PC runs
  Windows. So D11 (attach the PC to this cloud session) cannot work.
- Proposed to the user: run Phase B in a Claude Code session on the PC itself (desktop app, local), in its own folder
  cloned from https://github.com/ashersheets-jpg/crimson-desert-seamless-coop. That session has the game (D:), Chrome,
  MSVC/screen control (after GO); it starts from README_FOR_CLAUDE.md -> NOTES/STATE.md.

## 2026-10-31 D15: Local project folder on the PC
- The user asked Claude to choose the folder. Options: (a) keep the app's per-session folder (deleted with the session,
  path ~200 chars), (b) `C:\Users\asher\CrimsonDesert-SeamlessCoop`, the brief's suggested name, beside (not inside)
  the JumpRing/Elden Ring project.
- Chosen: (b). It survives the session, and the short path keeps upstream sources under Windows MAX_PATH for MSVC
  (attempt 2 failed with C1083 on a 260+ char path). We do not change the system long-path setting.

## 2026-10-31 D16: Nexus permissions verified; Comfort Pack approach unchanged
- All "ask first" mods stay idea-only (reimplement + credit); nothing is bundled. Permissive mods (Fat Stacks, Infinite
  Stamina, PrivateStorageAnywherePLUS, JMM, Barber 2.04) could be adapted with credit, but the co-op rule (D-policy 7:
  gameplay patches must match on both machines) still favours our own host-synced config.
- Live Transmog and CDUMM: GitHub says MIT, Nexus says ask first. We follow the stricter Nexus terms until the authors
  clarify; no code reuse for now.

## 2026-10-31 D17: M1 join flow uses Steam's own UI, not an in-game menu (for now)
- Options: (a) ImGui co-op menu with mouse/keyboard (needs a WndProc hook and input blocking, cannot be tested before
  GO), (b) Steam overlay for invites + "Join Game" + rich presence, with two hotkeys and a panel hint.
- Chosen: (b). Steam's overlay already is the friends picker players know; it needs no input hook; everything but the
  live flow is testable offline. The ImGui menu can come later (M2/M7) for settings.
- Mechanics: callbacks via SteamAPI_RegisterCallback (the exe pumps RunCallbacks), launch argument +connect_lobby,
  lobby id validated as a Chat-type CSteamID. A join ends the session and starts a guest session (CoopLink destructor
  restores hidden enemies and removes the puppet), then role settings are re-applied.
- Two players only (upstream's limit); lobbies stay friends-only, 2 members.

## 2026-10-31 D18: Party scaling as "damage taken / k", host side, experimental and off by default
- Options: (a) raise the enemies' max HP in memory (needs new RE of the HP pool and of boss phase thresholds; only
  possible live), (b) scale the damage enemies take: the guest's blows are divided by k before guest_hits applies them,
  the host's native blows get (1 - 1/k) given back through VaryStatCheatReq right after the VaryHPAck. Same result as
  health x k, built only from mechanisms upstream already uses live.
- Chosen: (b). A ledger of our own injected HP losses keeps the guest's blows from being scaled twice; kills are never
  undone; only hostile enemies in combat (enemy_relay) get anything back; the guest side never scales (host-auth).
- Risk: a positive VaryStatCheatReq delta (a heal) has never been sent in a live test. So `[scaling] enabled=0` by
  default, including in the package, until the M3 test shows it works (HP bar dips then recovers a little).
- Two players only for now: one value `health_2p` in cdmp.ini; config/scaling.toml takes over with integration step 2.

## 2026-10-31 D19: Save guard = automatic backup + warning, not blocking saves
- The sniffer is passive by design (it can observe and inject, not drop), and dropping SaveAutoReq could leave the
  game's save timers waiting. So: copy %LOCALAPPDATA%\Pearl Abyss\CD\save (5.4 MB here) to
  Documents\CrimsonDesert-CoopPlus-SaveBackups\auto-<time>-before-coop when a co-op session connects (at most every
  10 min, newest 10 kept), and log + show a panel notice when SaveAutoReq / SaveToFileCurrentPlayerReq goes out while
  connected. On by default ([coop] save_guard=1): it only copies files.

## 2026-10-31 D20: Integration step 2, first slice: our sim layer inside cdmp.asi for scaling curves
- Upstream's CMake builds our src/sim (+ toml++) into the mod when the tree sits inside Co-op+ (CDMP_COOPPLUS_SIM).
- The mod reads bin64\cdcoop\config\scaling.toml (our format, unit-tested loader); 2-player health for regular enemies
  and for bosses (boss_is_boss) via sim::compute (clamps, QoL compensation). cdmp.ini [scaling] enabled stays the
  master switch; the TOML can only turn scaling off. Other TOML files (loot, progression, comfort) wait for the
  features that use them.

## 2026-10-31 D21: [net] join = friends (default) | invite
- role=auto makes every Co-op+ player a host with a friends-only lobby and "Join Game"; upstream's host also accepts
  any friend who is playing. That is the convenient default (and upstream's behaviour), kept as `join=friends`.
- `join=invite`: the lobby is created private (k_ELobbyTypePrivate, joinable only through an invite), rich presence
  has no "connect" (no Join Game button), and the host locks only onto a Hello from a member of its lobby
  (GetNumLobbyMembers / GetLobbyMemberByIndex, exports checked). Room codes do not work in this mode (by design).

## 2026-10-31 D22: Port to game 1.0.0.3460 ourselves, without the old exe
- Steam updated the game 3446 -> 3460 at 08:54 (PE 0x6AE58024). The 3446 exe was overwritten, so the upstream method
  (diff the two builds) was not available, and nouzert1 had not ported yet (watched until 11:00).
- Each address was re-found by an identity that does not need the old bytes: upstream signatures (globals, packet
  hooks), RTTI class names (vtables, movement-method Enters at slot +0x38), recorded heads/context (layers: a call
  whose rel32 is identical in 3424/3446/3460 picks one of ~230 template instances), and a live read-only probe for
  the runtime Troc range (+0x4000, same 1175 handlers). offsets_3460.h builds on offsets_3446.h; verify-port checks
  all of it; the mod targets 3460 only (Steam auto-updates every player).
- Left at 0 (fails closed): kServerOtherEquipWriter -- its site bytes and prologue are shared by ~40 item-list
  writers and nothing offline tells them apart. Live gear changes are not relayed; gear at spawn still matches.
- Added a crash logger (vectored handler, logs only; process exit noted in DllMain) after the first launch closed
  the game with no trace. It did not recur in four launches.

## 2026-11-01 D23: Port to 1.0.0.3479; gear changes from an equipment watcher
- Steam updated to 3479 (2.04.05). Small patch; ported the same way as D22, now with the exe archived first
  (build/exe-archive) so later ports can diff. The dead equip-writer hook (a log formatter) is replaced by reading our
  own equip slots every 250 ms; verified live both ways. Overlay reads Streamline's swap chain class by RTTI (no
  throwaway swap chain): fixes a ~1-in-5 startup crash with DLSS frame generation.

## 2026-11-01 D24: Loot is per player, like Minecraft's Lootr (user request)
- The user asked for Lootr-style loot: every player can take every item for themselves; nobody's pickup takes it
  from the other. This is brief 6.4's default ("per-player instanced loot"), now confirmed by the user.
- How the design already gives it: each player plays in their OWN game and save, so chests and items in the world
  exist once per player -- opening "the same" chest gives each player their own contents. Enemy drops: on the host's
  kills the guest gets its own roll through the loot twin (loot_twin.cpp, [coop] guest_loot, default on); the host
  loots natively.
- To verify on two PCs: both open the same chest and both get the items; both loot the same enemy. Still to build
  (brief 6.4): currency and quest items to every nearby player; optional alternative modes (shared pile etc.).

## 2026-11-01 D25: Shared combat on by default
- Enemy copies, real hiding of the guest's natives in the zone, enemies targeting either player (2 at a time on the
  copy), hit reactions and lethal blows for the guest: all default on. Each was verified on one PC; the two-PC test is
  where they get confirmed. Every one has a [coop] key to turn it off.
- Loot for the guest from shared kills uses the guest's own hidden native of the same kind (no spawned twin: its
  request logs the player out on 3479).
- No friendly fire: hits between players are never relayed (by design; kept explicit, user request).


## 2026-11-01 D26: Our own revive prompt while the game's death screen stays away
- One-PC bisect (LOG, 21:00-23:50): once the other player's copy has been LOADED in our world, the game never opens
  its death screen for our player again -- not with the copy removed before the death, not with its user entity
  removed too, not with zero replayed events. Copy never loaded (`[dev] no_copy=1`) -> screen. The screen is a
  data-driven UI script (UIGamePlayControlReviveModal); its condition was not found in a few hours.
- Decision: in co-op, after a copy has been loaded and 4 s without the game's screen, the overlay shows the same
  choice and sends the game's own requests (net/revive_prompt.*). Q = Instant Revive works exactly as the game's.
  U = Revive at Checkpoint: the server revives us at the checkpoint (full health), but the client only moves there
  in its own loading flow; our DoTeleportAck took in 1 of 4 tests. Shipped as best effort (documented in README and
  the playtest checklist) rather than withholding U, because with no instant revives left U is the only way up.
- The earlier rule "the copy leaves while we are dead" is gone: it did not help, and the dead player keeps seeing
  the friend now.
- Possibly echo-only: the one-PC copy is a clone of our own character; the two-PC test must note which screen shows.

## 2026-11-01 D27: Opcode tables are checked against the live table after every port
- The relay table (kRelayedAcks) and two inline AttackByCollisionReq sites kept 3424 numbers on 3460/3479, so the
  copy got none of our HP/buff/death/revive messages and a lethal blow sent a guild request. Renumbered; new
  tools/port/check_opcodes.py fails on any hardcoded opcode whose name does not match the live Troc table, and on
  opcode tables inherited from an older build's header. Part of the port routine (C6).

## 2026-11-02 D28: Party scaling on by default
- One-PC check (echo, [dev] scaling_as_host=1): with 2 players the host's blows on bandits got 1/3 back (x1.5),
  relayed blows were not given back twice, enemies still died, no failed requests. The user asked for bosses scaled
  to the party (A8), so `[scaling] enabled` now defaults to 1 (the user's ini has no [scaling] section, so both
  players get it). Bosses (x1.7/2.3/2.8 from scaling.toml) and 3-4 players are not tested.
- The mod's own boss bar is now guest-only: the boss's health never reached it before (boss_note_hp had no caller),
  and with it fed the host would have seen two bars.

## 2026-11-02 D29: Boss-fight deaths (A9) on the revive prompt
- "In a boss fight" = a boss (health pool >= 500 HP, or the host's flag on a copy) whose HP changed in the last
  30 s. Dying then: no Q/U, "Your partner is still fighting the boss". Boss down after our death -> we get up where
  we fell with full health (the server's checkpoint revive without the move: reliable, unlike the move). The other
  player down too -> only U ("Everyone is down"). Partner disconnected or 30 s without boss HP changes -> normal box.
- Only possible where Co-op+'s box replaces the game's screen (always, so far, once a copy has loaded); if the
  game's own screen shows on two PCs, the wait cannot hold it (noted in the checklist).
- Verified on one PC with bandits as bosses ([dev] boss_hp=20, [dev] partner_alive=1): wait box, release after 30 s,
  everyone-down box + U. Not verified: the auto revive when the partner wins (needs two players).

## 2026-11-02 D30: The other player's horse is on by default
- One-PC echo rounds (build/port3479/horse_test.sh; H calls the horse, E rides, R dismounts on PC): the horse copy
  loads with its first action (no crash in 5 rounds; the 2976-era crash is contained), the copy MOUNTS its horse copy
  when we mount (RideOn fix), rides along (panel: 3 m away after ~80 m of riding), and dismounts with us.
  [coop] horse now defaults to 1 (the user's ini has no horse key). Other mounts and flying: see FLAGS F2.

## 2026-11-02 D31: Teleports in 40 m hops; Ctrl+F4 = go to your friend
- DoTeleportAck works for our own player up to ~50 m per message; longer moves are hops (revive_prompt.cpp hop_tick).
  Used by the checkpoint revive (now reliable on one PC) and by a new Ctrl+F4 "go to your friend" (beside them, only
  when both are alive). Very long distances (km) are untested: the world may not stream in time (two-PC check).

## 2026-11-02 D32: Shared cutscenes = notice + anyone's skip skips for everyone nearby
- Cutscenes play inside each client; the server gets no start/skip message (arm wrestling intro captures). Playing the
  host's cutscene on the guest would need the sequencer and its actors in the guest's world (often a different quest
  state): not done. What is done (net/coop_activity.*): the game's own "cutscene playing" flag (exe+0x70D2E2F, found
  by .data snapshot diff) is sent to the other player, who sees "<name> is watching a cutscene"; Esc during a cutscene
  is sent too, and the other player's mod presses Esc for them if they are in a cutscene within 100 m and the game is
  in front. "Everyone must press skip" would need to hold back the game's own Esc: not done (anyone's skip counts).
- boss_safe now takes the same flag: its SetAvoidByCutSceneAck check never matched on 3479 (18-byte message, not 21).

## 2026-11-02 D33: Minigames: each player plays in their own world; the other is told
- Minigames are client-simulated (ScriptFunctions_MiniGameData_*): the server only starts them, spawns the NPC
  opponent, and takes the result. A real player-vs-player match needs the minigame's live state (gauge, cards) in
  memory on both sides: research, not done. Now: "<name> is playing a minigame" / "<name> finished arm wrestling"
  toasts (ActivateMiniGameReq / LeaveMiniGameReq). No win/lose: MiniGameResultReq was identical for a win and a loss.

## 2026-11-02 D34: Long teleports try higher, shorter and sideways hops
- A 4 km trip stopped at a cliff wall (only height changes were tried); kHopTries now adds +130/+180 m, 15 m hops and
  30-40 m to either side: the same trip got past the cliff, then stopped in the sea (teleports do not take while
  swimming: swim to land and press Ctrl+F4 again).

## 2026-11-02 D32 (revised by the user): a cutscene skips only when everyone in it pressed skip
- While the other player is in a cutscene within 100 m, our Esc is held back (low-level keyboard hook) and only sent
  to them as "pressed"; when both pressed, each mod gives its own game the Esc. Banners: "waiting for <name>",
  "<name> wants to skip: press Esc". Alone in a cutscene (or the other player far away / not in one): Esc as normal.

## 2026-11-02 D35: Versus arm wrestling
- Both players at their own arm wrestling table (each in their own world, each vs the game's NPC as usual) and both in
  a match at the same time: the host's mod owns the gauge (0..100, the game's own strength gauge, found with a
  hardware breakpoint) = 50 + 2 x (host E presses - guest E presses) and writes it every tick; the guest writes the
  mirror. The game ends each match itself at ~96 / ~4, so the faster masher wins on both screens. The QTE "[R]" prompts
  of the NPC match play no part in the versus. Same mechanism should carry hand wrestling (same gauge), not tested.

## 2026-11-02 D36: Time of day follows the host by default ([coop] time_sync=3)
- It never ran on 3479: the FieldTime address was the 3446 one (fixed: exe+0x6EA4998) and the game clock it needed
  cannot be read (now: the capture's estimate). Only the time of day is matched (each keeps its own day count).

## 2026-11-02 D37: Weather follows the host ([coop] weather_sync=1, default on)
- Weather = server-chosen STAGES (Weather_Sunny / Cloudy_45..85 / Windy_Degree* / Windy_Speed* / Rainy_*), started
  with PrepareSequencerAck, random per load (9 loads of one save at one time: 9 different sets), so matching the time
  is not enough. The host sends its weather stages' PrepareSequencerAck payloads (EventKind::Weather 16, on change and
  every 5 s); the guest drops its own server's weather Prepare/Delete, ends the ones running once, starts the host's
  under sequencer ids 0x7E000000+n and answers its client's StageBranchStateReq / UpdateStageChartEventReq for them as
  the server does (its own server never heard of them). Leave / no report for 20 s: the guest's own come back.
  Verified on one PC ([dev] weather_test=19990030): own Windy+Cloudy_60 replaced by Rainy_Hard, rain icon + rain.

## 2026-11-02 D38: The host's stage cinematics are replayed in the guest's game ([coop] stage_cinema=1)
- Boss fights, story scenes and minigame intros are stages run by each player's own client; the boss stage only runs
  in the host's world, so the guest never gets its intro. On the host's cutscene start the stage is picked
  (SetAvoidByCutSceneAck's stage, else the stage that changed branch in the last 5 s; none = not shared) and its
  PrepareSequencerAck (bound actors translated to the guest's copies, enemy_copy_of) + branch messages go out as
  Activity types 7/8/9. A guest within 100 m starts a copy (sequencer 0x7D000000+n) at that branch, answers its chart
  requests, follows branch changes, and ends it with the host's cutscene (max 3 min, or on leave/loading).
- One PC ([dev] cinema_test, arm wrestling intro): picked 16005100 via SetAvoidByCutSceneAck, copy started 3 s in and
  ended with the cutscene; match continued normally. The client sent nothing for the copy (its own instance of the
  same stage was running): a real guest has no instance of the host's boss stage, which is the weather case (that
  one ran). Unverified until a two-PC boss test (FLAGS F3).

## 2026-11-02 D39: Rock, paper, scissors is a duel the mod runs (not the town minigame)
- The town RPS minigame is a stage: in its pick branch the client sends a transition per (our hand, child's hand)
  (seen: S-S 1, S-P 2, P-R 3, P-S 4, R-S 7, R-P 8; 9 = leave), but the child's hand comes from the stage's server-side
  random roll and the server ignores a transition that disagrees with it (rewrites to 6 and 5->3 got no answer). So
  player-vs-player through the child is impossible without editing the local server. The duel: U/F/E caught by the
  keyboard hook (kept from the game), Activity kRps carries each hand, reveal when both picked, first to 2.

## 2026-11-02 D40: Challenges (user: "give me the option to challenge a friend in the mini games")
- Shift+F5 (not Ctrl: Ctrl moves the game's interaction focus off the table host and crouches) at an arm / hand
  wrestling table invites to that game, elsewhere to an RPS duel; Shift+F5 accepts / withdraws / ends, Shift+F6
  declines; invites lapse after 60 s. Versus (shared gauge, duel) only inside an accepted challenge; series score.
  Activity kChallenge: u8 action (1 invite, 2 accept, 3 decline, 4 end) | u32 stage key.
- Verified on one PC ([dev] challenge_test=1/2, versus_test, rps_test): invite -> accept -> arm wrestling loss then
  win (series 0-1, 1-1 toasts); RPS duel win/draw/lose/win -> 2-1, series; incoming invite (Shift+F6 test trigger)
  -> Shift+F5 accept -> duel 2-0 -> Shift+F5 ends "Final series: you 1 - 0". Hand wrestling uses the same gauge path
  (no table reached). Two-PC test pending (PLAYTEST).

## 2026-11-03 D41: PvP duels (user: "1v1 PVP ... inside of a set radius ... out of bounds = automatic loss")
- A challenge game. Shift+F5 / chord + D-pad Up now opens a picker (the wrestling table we stand at, Duel,
  Rock-paper-scissors; arrows / D-pad Left-Right choose, held from the game while it is open; Shift+F5 sends,
  Shift+F6 cancels; closes after 20 s). Stage key of a duel: kDuelStage 0xD0E10001 (no game stage uses it).
- The two characters cannot hurt each other in the game (each is a friendly copy in the other's world; the PvP probe
  got no frame events naming the copy), so the attacker's mod judges hits: our StartActionReq (op 2202) whose action
  hash (payload +4, the same hash as the attack's DoFrameEventReq accessor: 28/28 matches in a 3479 recording) is a
  known attack (guest_hits seed/learnt table), 280 ms later, with the other player's drawn character within 3.2 m and
  65 deg of our facing (quaternion at kActionRot; forward = (-sin yaw, -cos yaw), yaw = 2 atan2(qy, qw), measured).
- The hit (hits_make_pvp: PlayerHit payload, attacker = 0xA010F001, our last learnt stagger) goes as Activity kDuel;
  the receiver takes 4..10% of its max HP (by the attack's base 0..40) through VaryStatByAccInputReq with the stagger,
  never below 3% (nobody dies). At 3%: round lost. Out of the 20 m circle (centre half way between the players at the
  start, placed by the inviter) for 10 s in a row: round lost. 3-2-1 / Fight! / Victory / Defeat title cards, the
  ring drawn on the ground (gold, red while out), the opponent's health as a game-style bar. After 6 s both heal back
  to their round-start health and the next round starts; series in the challenge.
- Activity type 12: u8 sub (1 round start: centre, radius / 2 hit: PlayerHit payload / 3 lost: why) | u32 round.
- Swing detection (revised the same night): the test character's attacks (left click 0x25DC7FB9, kick 0x626B7BEC)
  were in neither the seed nor the learnt table (those only know attacks seen hitting enemies). So an own
  StartActionReq also counts as a swing when an attack button (mouse left / F; pad RB, RT, Y) went down in the last
  350 ms, with a middling base (15). Pad RT/R2 is now read (kPadRT 0x0800: XInput trigger > 100, Sony b1 0x08).

## 2026-11-03 D42: Shared progression = a party journal (quest progress itself cannot be pushed)
- Each player's quests are advanced by their own in-process server from their own world; requests from outside
  (CompleteQuestByCheatReq, LearnKnowledgeByCheatReq...) get no answer on retail. Quest steps are server-side
  (UpdateMissionListAck 2360 lists missions: u32 key | u8 state 2 under way / 5 done | u8 kind; login sends all,
  then 4-4.4 KB updates). Story progress therefore stays per save; what CAN be shared and is: the journal.
- party_quests: our server's mission states are watched; a mission going 2 -> 5 is sent to the other player
  (Activity 13: u32 key); they get a game-style notice "<name>: Quest Updated" / "Challenge Complete" with the
  game's own name (mission_names.inc, 5447 names generated from the game data by tools/gen/mission_names_inc.py),
  marked "you are on this quest too: finish it together" when their own copy is under way. Walking an objective
  together (same place / dialog / fight) does finish it in both worlds; kill objectives on the host's enemies do
  not (the guest hits copies; FLAGS F8).

## 2026-11-03 D43: Flying mount verified (Blackstar) with a test save that has the rope skills
- The dragon test save summoned Blackstar but Kliff never got on: it lacked Knowledge_Rope_AirPull 1003512,
  Knowledge_Rope_AirSwing 1003513, Knowledge_Rope_Swing 1001711 (and Knowledge_Unique_Varnia_Dragon 1000560).
  Swapped in for four rest-area discoveries (tools/save/knowledge_swap.py) -> test save "dragon-rope": F1 wheel ->
  Blackstar -> automatic grapple -> riding (W forward, Space climbs, prompts Fireball / Fire Breath / Aim Q / Land /
  Dismount R). ~22 m/s; climbing drains the dragon's stamina bar (blue, under the minimap); empty = it descends and
  drops you (at 1346 m after ~70 s of climbing). flyto.py (build/port3479) steers it.
- Route to Delesyia for the wyvern: the flight reached Wells Estate / Mistveil Forest (-8660, 453, -4474), 2.1 km from
  Wyvern's Cradle, past the bay that blocked every walking/teleport route. Next: fly level (stamina), land to rest.
- Delesyia reached (2026-11-03 00:30): test save wyvern-dragon / wyvern-egg-dragon (= dragon-rope, start moved to
  the nest's revive point; the game loads it at Trembling Gorge (-9004, 479, -5066)). Summon Blackstar (it needs open
  ground: twice it did not come in the forest right after loading; a second wheel use worked), flyto.py -6736 -5330:
  2.3 km in ~100 s, "Wyvern's Cradle" / "Three Brothers Cliff Wyvern Nest - Knowledge Acquired". Dismounting 290 m up
  (R) killed Kliff (no glide); "Revive at Checkpoint" puts him at the nest's revive point (-6727, 533, -5305).
  tools/save/move_save.py now finds the position field in saves of other sizes.
