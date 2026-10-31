# Shared progression (M5) design

Brief 6.3: shared by default story/quest progress, region discovery, map unlocks, key world events; personal
inventory, equipment, skills, appearance; all in `config/progression.toml`; always back up saves; never overwrite a
guest's single-player save without clear consent. Status 2026-10-31: design from offline research; every message
format below still needs a live capture (Phase B).

## What we know (upstream research, build 2976; opcodes checked equal on 3446 for the ones the mod already uses)
- Each player keeps their own world and save; the guest's world is driven by the guest's own in-process server.
  Nothing the host's server does reaches the guest's save unless the guest's mod tells the guest's server.
- The game's internal server has the messages we need, both normal and developer ("cheat") ones
  (`docs/research/data/troc_messages_2976.csv` in upstream):
  - knowledge / discovery: `AcquireKnowledgeReq` (2964), `ShareKnowledgeDataListReq` (2258, native!),
    `LearnKnowledgeListAck` (2103), `NormalLearnKnowledgeListAck` (2439), `LearnKnowledgeByCheatReq` (2155),
    `LearnKnowledgeGroupByCheatReq` (2390), `StartDiscoverLevelSceneObjectReq` (2154),
    `UpdateDiscoveredLevelGimmickSceneObjectDataAck` (2487);
  - quests / missions: `UpdateQuestListAck` (2808), `UpdateMissionListAck` (3009), `SubMissionProgressListAck`
    (2102), `NotifyQuestSaveDataAck` (2426), `CompleteQuestGaugeAck` (2738), `CompleteChallengeAck` (2118),
    `CompleteQuestByCheatReq` (2627), `ResetQuestByCheatReq` (2251);
  - saving: `SaveAutoReq` (3037), `SaveToFileCurrentPlayerReq` (2910) (used by the save guard, D19).
- The cheat requests write no global or profile flag (upstream `pendiente-recompensas-progreso.md` §2.6, verified for
  KillCheatReq / VaryStatCheatReq); telemetry is sent only for a PLAYER's death. Party scaling already relies on
  VaryStatCheatReq the same way guest_hits does.
- Camp liberation gauges advance on any death in that world, whoever caused it; quest "kill" objectives and skill XP
  need a native kill (upstream §2.4, §3).

## Approach, safest first (each tier is a separate switch in config/progression.toml)
1. **Discovery and knowledge (`region_discovery`, `map_unlocks`)**: additive and order-free. The host's mod observes
   the knowledge it learns (S>C `LearnKnowledgeListAck` / `NormalLearnKnowledgeListAck`), sends the keys over the
   reliable channel, and the guest's mod asks its own server for the same knowledge (first try the native
   `ShareKnowledgeDataListReq`, else `LearnKnowledgeByCheatReq`). Both directions, so a guest's discoveries reach the
   host too (`guest_progress_counts_for_host`). Never removes anything.
2. **World events and camps (`world_events`)**: mirror completed quest gauges / camp liberations to the other world
   when both players were there (host sends the gauge key on `CompleteQuestGaugeAck`).
3. **Story and side quests (`story`, `side_quests`)**: highest risk: quests are scripted stages, and completing one
   out of order in the guest's world could strand a later step. Plan: only mirror a quest the guest ALSO has active
   (from the guest's own `UpdateQuestListAck`), only on completion, only with the guest's consent for the session
   (a panel prompt), and only after the save guard has a fresh backup. Default for the first release: off, even
   though the brief says shared; turned on per milestone test.
4. **Guest save mode** (`guest_save_mode`): the safe default remains what upstream does (each in their own save);
   `separate_slot` needs the save-slot messages (`SaveDataSlotRefreshReq`, ...) understood live.

## Phase B captures needed (one PC, `[sniffer] record=1`, upstream tooling reads the .bin)
- Learn knowledge by talking to an NPC and by reading a document; discover a new region; unlock a map area.
- Complete a short side quest; liberate a small camp (the gauge and the batch that follows it).
- Then: payload layouts in docs/protocol.md, a reliable-channel message `ProgressShare` (protocol bump), and the
  observer/injector module (`net/progress_share.*`), with unit tests for the key lists.
