# Proven patterns from other co-op mods (design lessons for Crimson Desert)

Date retrieved (all sources): 2026-10-31. Status: COMPLETE for this pass.

Access limits: nexusmods.com, reddit.com, ersc-docs.github.io, wiki.tiltedphoques.com, pcgamer/dualshockers etc. were NOT fetchable. Only github.com / raw.githubusercontent.com were read directly (confidence HIGH there). Everything else is from WebSearch result summaries (confidence MEDIUM/LOW, marked per item).

## Key takeaways

1. **Lobby = shared secret, nothing else must match except versions.** Elden Ring Seamless Co-op: everyone sets the same `password` in `ersc_settings.ini`; host opens session with an in-world item, guests join with another item, a third item ends the session. Kenshi Coop: paste Steam IDs, pick HOST/JOIN, go ONLINE. Crimson Desert precedent: cdmp = Steam friends auto-discovery; Nexus 3814 fork = room code with encrypted relay. -> Recommend: room code / password + Steam friends, both visible in an overlay.
2. **The host's config wins for world rules.** In Seamless Co-op the host's ini decides scaling, invasions, rot, spirit summons; only the password must match across players. -> Same for Crimson Desert: host-only difficulty/scaling knobs, guest config only for local UI.
3. **One owner per entity, explicit ownership transfer.** Skyrim Together Reborn: each synced actor has ONE owner client that runs real AI/physics and streams it; others display a remote copy; ownership moves when the owner leaves the cell or disconnects; players always own themselves; IDs are `GameId` (mod id + base id), never runtime form IDs. Its changelog shows "wrongful ownership transfer", "double applying of RemoteComponent", "double queue of event" as recurring bugs. cdmp already uses host-authoritative enemy copies (the Genshin model). -> Use host-assigned stable net IDs from spawn data; never pointers or per-load actor keys.
4. **Session persistence through death and boss kills is the feature players love most.** Seamless Co-op: dying does not end the session; spectate during bosses (ini `always_spectate_on_death` for everywhere); bosses/areas no longer send co-op partners home. DS3 Seamless: respawn at your last bonfire in the same world, session continues.
5. **Separate co-op save files + strict version gate.** Seamless writes `.co2` saves instead of `.sl2` (vanilla untouched). Kenshi Coop: any save either player makes becomes one shared save streamed to both; if the joiner has an identical save no transfer. Skyrim Together: server rejects any client whose `BUILD_COMMIT` differs; "Deny load for unsupported game versions"; server list marks compatibility. -> Crimson Desert mods already hit this (cdmp pinned to 1.0.0.2976; save damage if saving while partner is in your world; no version check so mismatched kits "search forever").
6. **Quest sync = leader/host-driven, stage-based, with a debugger.** Skyrim Together: quest progress comes from the party leader; "quest sync through stages" + "SetStage debugger"; quest-item sync flipped on/off between versions and caused duplication bugs; misc quest sync is an opt-in server flag (`bEnableMiscQuestSync`); dialogue fixes when a non-leader is the speaker. Wait/sleep disabled to keep time aligned.
7. **Things players hate:** every game patch breaks the mod (Seamless broke at Shadow of the Erdtree launch and at 1.12); boss HP scaling felt too harsh to some; removing/altering invasions is divisive; infinite loads, quest-progress glitches, scene-swap bosses and teleport traps desync (Seamless); NPCs vanishing mid-quest, enemies all targeting the leader/host (Skyrim Together, later fixed by spreading aggro).

## Raw log by mod

### Elden Ring Seamless Co-op (LukeYui) -- confidence MEDIUM (guides + Steam threads via search; official docs/Nexus not fetchable)
- Nexus: https://www.nexusmods.com/eldenring/mods/510 ; docs: https://ersc-docs.github.io/how-to-install-and-update/ , https://ersc-docs.github.io/seamless-modding/ (search snippets only)
- Guides: https://www.dualshockers.com/elden-ring-seamless-how-does-co-op-mod-work/ ; https://www.pcgamer.com/playing-elden-ring-with-friends-the-seamless-co-op-mod-is-the-way-to-do-it/ ; https://indieforgames.com/elden-ring-seamless-coop-mod/
- Lobby/password flow: `ersc_settings.ini`; "The only setting that has to be the same for everyone in the play group is the password" (docs snippet). Launch via mod's launcher. Tiny Great Pot = open/host session; Effigy = search for session with your password and join; Separation Mist = send players home / end session. Advice: use a strong password, anyone with it can join.
- Session persistence: dying keeps you connected; spectate in boss fights (ini `always_spectate_on_death`); boss kills / area clears don't send co-operators home. Up to 4 players (1 host + 3) per guide snippet (other snippets mention larger counts; unverified).
- Scaling: per-additional-player % for enemy health, damage, posture, boss health (one guide: enemy health 35%/player, boss health 100%/player -- defaults differ between sources). Host's ini decides world scaling, invasions, rot, spirit summons (Steam thread snippet https://steamcommunity.com/app/1245620/discussions/0/601910715107850518/).
- Save handling: separate `ER0000.co2` saves; vanilla `.sl2` untouched.
- Trade-offs: no FromSoftware official servers while active; all players must have same mod + game version; updates break it (https://pcgamer.com/games/rpg/elden-ring-shadow-of-the-erdtrees-release-has-busted-the-seamless-co-op-mod-but-its-creator-has-a-fix-incoming-as-soon-as-possible ; Yui promised saves would not be lost).
- Progress sharing: guides claim shared world progression (boss kills etc.) -- NOT verified from authoritative docs; one snippet explicitly contrasted vanilla "progress saved only for host". Treat as LOW.
- Complaints: infinite loading screens, crashes, quest-progress glitches, scene-swapping bosses and teleport traps desync, avoid Roundtable Hold without severing first (Steam thread snippets); boss HP scaling "much harder"; invasions debate.

### Dark Souls 3 / Dark Souls Remastered Seamless Co-op (LukeYui) -- confidence MEDIUM
- https://www.nexusmods.com/darksouls3/mods/1895 ; https://80.lv/articles/experience-dark-souls-3-with-friends-using-seamless-co-op-mod ; https://www.pcgamesn.com/dark-souls-3/seamless-coop-mod ; DSR: https://www.timeextension.com/news/2025/04/dark-souls-remastereds-new-seamless-co-op-mod-lets-you-play-with-friends-without-interruptions
- Death -> respawn at last rested bonfire in the same world, session continues. Up to 6 players (host + 5). No FromSoftware matchmaking; own save files. "Progression made in online play will also affect progress in each player's own world" (news snippet). Launched alpha, buggy.

### Skyrim Together Reborn (Tilted Phoques, TiltedEvolution) -- confidence HIGH for repo docs, MEDIUM for Nexus changelog/news
- Repo: https://github.com/tiltedphoques/TiltedEvolution (GPLv3). Read directly: `AGENTS.md` https://raw.githubusercontent.com/tiltedphoques/TiltedEvolution/master/AGENTS.md and `CHANGELOG.md` (top entry 1.38.3, 2022-07-01) https://raw.githubusercontent.com/tiltedphoques/TiltedEvolution/master/CHANGELOG.md
- Architecture: clients run the simulation; server keeps a light world model (players, parties, cells, which actors/objects exist, who owns them) and routes updates; never simulates Skyrim; clients trusted, server validates parties, ownership, admin commands. 2-8 players. Dedicated server runs on Linux too.
- Ownership: one owner client per actor/object runs real AI/physics and streams; others show a remote replay copy; ownership moves when owner leaves cell or disconnects; players always own themselves. Changelog 1.34.0: "party leader hosts all actors", "transfer ownership to party leader on creation", fix "wrongful ownership transfer".
- Quests: progress, weather, time come from party leader. 1.29.0 "quest sync through stages" + "SetStage debugger"; 1.24.0 "dont sync quest items" then 1.29.0 "sync quest items"; 1.33.0 "duplicating quest items", "quest items and lock sync bugs".
- Versioning: "server rejects any client whose `BUILD_COMMIT` doesn't match" (no backward compat => protocol changes free); 1.12.0 "Deny load for unsupported game versions"; 1.15.0 "Use address library instead of hardcoded addresses"; 1.28.0 "mark servers as in/compatible in server-list".
- IDs: "Use `GameId` (mod ID plus base ID) in messages, never raw runtime form IDs." Movement unreliable, everything else reliable by default.
- What broke (changelog): WaitingFor3D spawn bug fixed repeatedly; "entt corruption bug"; "double applying of RemoteComponent"; "double queue of event"; "invisible players"; "missing player characters"; doors ("the door problem (again)"); "actor not dying after kill move"; "enemies not dying after hitting 0 hp"; interior worldspace crashes.
- Recent Nexus changelog (search snippet, "about 9 days old"; https://www.nexusmods.com/skyrimspecialedition/mods/69993): NPC ownership transfer "refined and stabilized" (fixed NPCs appearing unclothed); dialogue sync fixed when non-leader is participant; quest progression in parties reworked to reduce broken quests/softlocks; experimental `bEnableMiscQuestSync` server-side; quest start code "only partially fixed"; aggro now spreads more evenly. Older reviews: only leader's interactions matter for quests; enemies mostly targeted leader/host; physics items slid around; wait/sleep disabled (survival mode doesn't work). (https://www.thegamer.com/skyrim-together-reborn-multiplayer-mod-good-to-great/ , https://www.itechpost.com/articles/112040/20220712/skyrim-s-multiplayer-mod-mostly-working-despite-problems.htm)

### KenshiCoop (nhoral) -- confidence HIGH (README read directly)
- https://github.com/nhoral/KenshiCoop ; wiki https://github.com/nhoral/KenshiCoop/wiki
- RE_Kenshi/KenshiLib plugin; all game-state changes on main thread; ENet UDP + optional Steam P2P tunnel (no port forwarding); direct UDP via `coop_config.json`.
- "The host is authoritative for the world; each client is authoritative for its own squad."
- Join: F2 panel (works at main menu); paste Steam IDs (not saved to disk); HOST/JOIN + ONLINE. **Host streams its world snapshot on connect** with progress banner; skipped if joiner already has identical save.
- Saves: any save by either player becomes one shared save streamed to both; resume = host loads + online, joiner reconnects from menu.
- Syncs squads, NPCs, combat, inventory/equipment, trades, dropped items, base building, containers, shared money pool, game speed. Version mismatch -> "protocol mismatch" in log. Two players only. Expects "rough edges, desyncs, and crashes"; no desync recovery described.

### Kenshi Online -- confidence LOW (hosting-provider guide snippets)
- https://client.citadelservers.com/knowledgebase/1313/Kenshi-Online-Multiplayer-Client-Guide-How-to-Play-and-Connect-to-a-Server.html : ENet, positions synced every 50 ms, combat server-authoritative (damage computed on server and broadcast).

### Genshin Impact co-op (official game, cited by cdmp) -- confidence MEDIUM (only via cdmp index)
- cdmp `docs/research/genshin-coop-referencia.md` cites Genshin co-op as host-authoritative precedent: guests visit host world, host's enemies are authoritative. (Listed in https://raw.githubusercontent.com/ByJunter/CrimsonDesertMultiplayer/main/docs/research/README.md)

## Design lessons mapped to Crimson Desert

| Topic | Proven pattern | Crimson Desert implication |
|---|---|---|
| Lobby flow | Password/room code (Seamless), Steam ID paste (Kenshi), auto friend discovery (cdmp) | Offer Steam friends + room code (Nexus 3814 already does encrypted room-code relay). Show version/build in lobby; refuse mismatches loudly (cdmp lacked this). |
| Entity ownership | Single owner per actor + transfer on leave/disconnect (STR); host-authoritative world (Kenshi, Genshin) | Host owns all enemies near either player; guest shows puppets; guest hits sent as requests. Needs stable IDs from spawn data (actor keys change per load in CD). Handle ownership handoff when host loads a new area. |
| Guest's own world | Seamless/Genshin: guest is IN host's world | CD guest still runs own local server + own enemies -> must suppress guest's native hostiles in shared zone (cdmp phase 5, 150 m). |
| Scaling | Per-player % HP/damage/posture/boss HP, host's config wins | Add host-side scaling knobs; boss HP default must be conservative (complaints about harshness). |
| Persistence | Death != session end; spectate on boss death; no send-home after boss | CD respawn currently restored guest's enemies and killed guest (cdmp `zona-tras-muerte-anfitrion.md`); design death/respawn as session-preserving. |
| Saves | Separate co-op saves (.co2); single shared save streamed (Kenshi) | Block or sanitize saving while puppet is present (cdmp/nouzert1 warn of save damage); consider separate co-op save slot. |
| Quests | Leader/host-driven, stage-level sync, debugger, opt-in for risky quest types | Relay host quest stage messages to guest (issue #51 plan using `CompleteQuestByCheatReq`/stage updates); start with main-quest stages behind a flag; build a stage debugger. |
| Versioning | Strict build-hash gate; address library/AOB not hardcoded | CD patches already broke cdmp twice; use AOB + RTTI vtable lookup (nouzert1), refuse unknown builds, show it in UI. |
| Desync | Duplicate-event guards, idempotent apply, re-stamp timestamps | cdmp: partial reliable messages stuck forever, out-of-order enemy msgs, no lifecycle across loading screens -> add world-epoch gate (proposed in cdmp P5) and resync snapshot on load (cf. Kenshi snapshot, No_Variation_7024's world-state snapshot). |

## Not found
- No authoritative statement of exactly which world flags Seamless Co-op shares (boss kills / NPC quests); guides conflict.
- No primary Seamless Co-op design write-up from Yui (docs site not fetchable).

## Impossibility / difficulty claims (from other co-op mods)

| Claim | Source | Reasoning given | What would have to be true for it to be really impossible |
|---|---|---|---|
| Wait/sleep can't work in co-op | Skyrim Together reviews (snippets) | Time skips desync players' world clocks | Time could not be driven by a single authority. STR already syncs time from the leader; a vote/all-ready skip is a design choice, not impossibility. |
| Only the leader's interactions advance quests | Skyrim Together reviews | Quest state owned by one client to avoid conflicts | Per-client quest scripts could not be forwarded to the owner. It's a deliberate authority simplification. |
| Scene-swap bosses / teleport traps break co-op | Seamless Co-op Steam threads | Engine moves player between maps outside normal flow | Transition could not be detected or mirrored for peers. Workarounds exist (sever and rejoin). |
| Mods break on every game patch | Seamless (SotE, 1.12), STR (version deny), CD cdmp | Hardcoded addresses/protocol drift | Only if patches were obfuscated per-build so signatures could not be re-found; address libraries/AOB scanning mitigate. |
| Kenshi desyncs expected | KenshiCoop README | Hobby project, no conflict resolution | N/A -- a maturity statement, not a technical limit. |
