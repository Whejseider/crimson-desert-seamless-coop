# Protocol notes for the co-op backlog (1.0.0.3479)

Field sizes are what each Req's handler (vf2) reads, in order (build/port3479/reqsizes.py). Opcodes in
docs/research/game-features-troc.md. "u8/u16/u32/u64" by size; vec3 = 12 bytes; meaning is a guess until a capture
confirms it.

## Minigames
| Req | Fields | Guess |
|---|---|---|
| JoinMiniGameReq | u8, u16, u32, u32, u64, u8, u64 | 04?, minigame id, actor (NPC/table), ?, ?, mode, ? |
| ActivateMiniGameReq | u16, u32 | minigame id, ? |
| ApplyMiniGameReq | u16 | minigame id |
| DeactivateMiniGameReq | u16 | minigame id |
| SpawnMiniGameCharacterReq | u8, u64, u16, u32, u32 | 04?, ?, minigame id, character (opponent) key/variant, ? |
| UnspawnMiniGameCharacterReq | u8, u64, u16, u32 | |
| MiniGameResultReq | u16, u8, u32 x4 | minigame id, win/lose, scores |
| PushMiniGameRewardReq | u16, u32, u64 | minigame id, item/currency, amount |
| RecoveryActorForMiniGameReq | u16, u32 | restore the player after (HP etc.) |
| LeaveMiniGameReq | u8, u32, u32, u64, u8, u8, u32, u64 | |
| UpdateMiniGameLeaderboardDataReq | u16 | |
| UpdateMiniGameLeaderboardPlayerNameReq | u16, u32, u32 | |
Acks: JoinMiniGameAck, LeaveMiniGameAck, SpawnMiniGameCharacterAck, UnspawnMiniGameCharacterAck,
MiniGameRecordListAck, UpdateMiniGameLeaderboard*Ack, RefreshMiniGameBanDataAck. Rooms: CreateRoomAck, JoinRoomAck,
LeaveRoomAck (Duo tables?).
Co-op plan: capture each game (FLAGS F4). MIRROR first: relay Join/Spawn/Result so the friend's world spawns a copy of
the opponent next to the player's copy and sees the actions (StartAction relay already covers the player). VERSUS:
both join the same minigame id; exchange MiniGameResultReq scores. JOIN (PvP): replace the opponent with the other
player -- needs each game's live state (gauge for arm wrestling, targets for archery) found in memory.

## Cutscenes / stages
| Req | Fields | Guess |
|---|---|---|
| LetterBoxEnterReq | u32, u64 | player key, cinematic/sequencer id -> the "a cutscene starts" signal |
| LetterBoxExitReq | u32 | player key -> ends (skip or finish) |
| StartPlayerBehaviorSequencerReq | u32, u32 | player, sequencer id |
| PreLoadSequencerReq | (none) | sent before loads; answered by PlayAllPreLoadSequencerAck (seen at every load) |
| StageQuitReq | u64, u32 | stage id, reason (quit = skip?) |
| ForcePlayStageSequencerDevReq / ForcePlayStageDevReq | dev | could force-play a sequencer by id in the other world (risky) |
Ack side: SetAvoidByCutSceneAck (u8 4 | u32 user | u64 stage id | u8 flag | u32 user), SetGlobalStageSequencerPlayDataAck,
StageQuitAck, SequencerStageFailAck. Plan in COOP-FEATURES 4.x; blocked on a capture (FLAGS F3).

## Party (MMO leftovers)
OpenPartyReq u32,u32,u32,u8; InvitePartyMemberReq u32,u32; JoinPartyOfReq u32,u32; LeavePartySelfReq (none).
The copy's user (0x9010F001) exists only in the client (injected), not on the local server, so a server-side party
with it cannot form. A client-only party UI would need injected OpenPartyAck/JoinPartyAck; unknown if the party UI
survives in the single-player client. Low priority.

## Map, weather, difficulty, knowledge
- CreateOrChangeSelfDestinationReq: vec3, u8 -> our custom waypoint. Shared waypoints: relay to the partner and show
  it there as a second pin (CreateOrChangePinMarkerReq: u64 id, vec3, u8, u8, u8 -- the mod's partner pin uses the
  pin-marker Acks already).
- SyncWeatherStateReq: 6 x u32 -> the client reports its weather to the server (weather looks client-driven).
- ChangeGameDifficultyReq: u32 difficulty.
- ShareKnowledgeDataListReq: u32; AcquireKnowledgeReq: (none). Knowledge sharing: capture needed.

## Mounts
- RideOnVehicleReq u32, u32, u8 (vehicle, seat?, ?). Upstream (2976): never sent for horses; mounting goes through
  the interaction system. Horse copy works on 3479 (D30).
