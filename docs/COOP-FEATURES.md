# Co-op feature backlog (everything not yet co-op), 2026-11-02

Sources: user requests (2026-11-01/02), docs/research/game-features-web.md (wikis, guides), the game's own message
table grouped by system (docs/research/game-features-troc.md, 1,175 messages on 1.0.0.3479), and what the mod
does today (NOTES/STATE.md). Each item: what co-op should mean, status, approach, key messages, and how it can be
tested. Test column: 1PC = can be built and checked on one PC (echo); 2PC = needs the playtest; RE = needs
reverse-engineering first. Status: done / partial / todo / flagged (NOTES/FLAGS.md).

Model reminder: each player plays in their OWN world and save. The guest sees the host's character and enemies
as copies driven by relayed packets. "Co-op for feature X" therefore means one of:
  (a) MIRROR: the other player SEES it (relay the actions/visuals onto their copy), or
  (b) SHARE: both players' worlds get the same state/result (apply it in each world), or
  (c) JOIN: both take part in one instance (one world runs it, the other plays through copies), or
  (d) VERSUS: both play their own instance and the results are compared/raced.

## 1. Mounts and vehicles  (user: "guests can ride mounts, flying and on the ground", "mounts work for both")
| # | Feature | Co-op meaning | Status | Approach / messages | Test |
|---|---|---|---|---|---|
| 1.1 | Horse riding: see the other player ride | (a) | DONE on one PC (D30): copy mounts its horse copy, rides along, dismounts; on by default | remote_puppet horse load + RideOn/MoveActor relays; enable, fix, default on | 1PC |
| 1.2 | Horse mounted combat (blows from horseback on shared enemies) | (a)+(b) | todo | DoFrameEventReq from rider; same path as guest_hits | 1PC/2PC |
| 1.3 | Other ground mounts: bears, direwolf, raptor, lion, cow, red dodo, giant iguana | (a) | todo | same load path as horse (type differs); mounts are "mercenaries"/vehicles (CallVehicleMercenaryAck, ReserveSummonAndRideAck) | 1PC if one is reachable |
| 1.4 | Flying: Blackstar Dragon (Chapter 11), Wyvern?, Cloudcart balloon | (a) | flagged F2 (needs a save with a flying mount) | flying move data (3D, no ground snap); test needs a save with the dragon | flagged: needs a late save |
| 1.5 | Vehicles: ATAG mech, Pywel Wagon, Skiff, Delesyia train | (a) | todo | AttachLinkVehicle*, RideOnVehicleReq, ChangeVehiclePhysicsState* | RE + late save |
| 1.6 | Ride together (passenger seat, both on one horse/dragon) | (c) | todo | ChangeRideSeatAck exists: the engine has seats; research if a second seat is usable | RE |
| 1.7 | Guest summons own horse in host's world | (b) | done by 1.1 (each side shows the other's horse); 2PC check | guest's horse is in guest's world; host sees copy via 1.1 | 1PC |
| 1.8 | Ships / naval (DLC "Charting the Unknown": crew stations) | (c) | todo | ShipCrewStationChangeReq; only if DLC installed | flagged: DLC |
| 1.9 | Taming wild horses/bears with the other watching | (a) | todo | taming minigame actions relayed | 1PC |

## 2. Minigames  (user: "wrestling, archery, arm wrestling... get all of those to work")
Game protocol (all minigames): JoinMiniGameReq/Ack, ActivateMiniGameReq, ApplyMiniGameReq, SpawnMiniGameCharacterReq/Ack
(the NPC opponent), MiniGameResultReq, PushMiniGameRewardReq, LeaveMiniGameReq/Ack, UpdateMiniGameLeaderboardData*,
MiniGameRecordListAck, RefreshMiniGameBanDataAck, RecoveryActorForMiniGameReq; "rooms": CreateRoomAck/JoinRoomAck/
LeaveRoomAck (likely card tables). Plan per game: first MIRROR (the friend watches you play, opponent + your moves
visible), then VERSUS (each plays the same contest at the same time, best score wins, both see both scores), then
JOIN where the game has two sides (the friend takes the NPC opponent's place) -- needs RE of each game's state.
| # | Game | Where | Co-op target | Status | Test |
|---|---|---|---|---|---|
| 2.1 | Arm Wrestling | Hernand Inn | JOIN (player vs player: each mashes, gauge = difference) | todo, RE | 1PC capture, 2PC |
| 2.2 | Shot Contest: Bow | Lioncrest Manor | VERSUS (same targets, first to 10) then JOIN | todo | 1PC capture |
| 2.3 | Shot Contest: Gun | City of Hernand | VERSUS | todo | 1PC capture |
| 2.4 | Wrestling Match (grapple only) | Kharonso | JOIN (the friend as opponent: PvP under contest rules) | todo, RE | 2PC |
| 2.5 | Unarmed Duel: Boxing | Goldenfist Arena | JOIN | todo, RE | 2PC |
| 2.6 | Weapon Duel: Spear | City of Hernand | JOIN | todo, RE | 2PC |
| 2.7 | Duo (card game, up to 4 seats) | Hernand Inn 2F, Beighen | JOIN (both at one table) | todo, RE | 2PC |
| 2.8 | Rock Paper Scissors | Hernand (child) | JOIN (vs each other) | todo | 2PC |
| 2.9 | Horse racing / "Race Horse" | ? | VERSUS (race each other) | todo, research | 1PC capture |
| 2.10 | Sparring with a Wandering Freesword, All-Out Duel, Hand Wrestling, Spearsmanship Contest | various | as above | todo, research | - |
| 2.11 | Leaderboards / challenges ("Hero's Handshake I", "A Bloom of High Stakes") | - | SHARE: show both players' records | todo | 1PC |
| 2.12 | Friendly PvP duel anywhere (opt-in) | - | new feature on top of A11 (no friendly fire by default) | todo | 2PC |

## 3. Story, quests and progression  (user: "group/party progression in the story")
| # | Feature | Co-op meaning | Status | Approach / messages | Test |
|---|---|---|---|---|---|
| 3.1 | Party: a real group (invite, list, leave) | (b) | todo | the engine still has OpenPartyReq/InvitePartyMemberReq/JoinPartyOfReq/LeavePartySelfReq/DeletePartyMemberAck: try a party UI with the copy as member; else our panel | RE |
| 3.2 | Shared quest progress (host's quest step completes for the guest too) | (b) | todo, research | UpdateQuestListAck, NotifyQuestSaveDataAck, CompleteQuestGaugeAck; CompleteQuestByCheatReq exists (risky: order matters); design docs/progression-sync.md | RE, 2PC |
| 3.3 | Quest dialogs seen together (talk to an NPC, the friend hears it) | (a) | todo | SetQuestDialogReq/Ack, ShowAIDialogAck relayed to the copy's world as a subtitle box | 1PC |
| 3.4 | Main-story stage fights (bosses in stages) | (c) | partial: boss safety + A9 | stage sequencer state; guest can't enter host's stage instance | 2PC |
| 3.5 | Knowledge/insight shared (learn once, both learn) | (b) | todo | ShareKnowledgeDataListReq/ShareKnowledgeSeedAck, AcquireKnowledgeReq, FollowLearn* (built-in "learn by following") | 1PC capture |
| 3.6 | Experience / skill points shared on shared kills | (b) | partial (guest loot) | AddExperienceAck; give the guest XP for assists | 2PC |
| 3.7 | Challenges completed together | (b) | todo | CompleteChallengeAck, SyncChallengeAck | 2PC |
| 3.8 | Map discovery / fog shared | (b) | todo | FogDataAck, RegisterVisitZoneReq | 1PC capture |
| 3.9 | Collectibles (Abyss artifacts) | per player (Lootr rule D24) | by design | each world has its own | - |
| 3.10 | Difficulty: host's difficulty applies | (b) | todo | ChangeGameDifficultyReq | 1PC |
| 3.11 | Boss Rush mode together | (c) | todo | ReadyBossRushReq | RE |

## 4. Cutscenes  (user: "play for everyone within 100 m of whoever started it, skippable by everyone's skip button")
| # | Feature | Status | Approach / messages | Test |
|---|---|---|---|---|
| 4.1 | Detect a cutscene start/end on either player | todo | LetterBoxEnterReq/LetterBoxExitReq (client tells the server it shows the letterbox), SetAvoidByCutSceneAck (stage cinematics), StartPlayerBehaviorSequencerReq/Ack | 1PC capture |
| 4.2 | Play it for the other player within 100 m | todo, RE | the cutscene is a stage sequencer in the starter's world; the other world lacks that stage state. Options: (i) force-play the same sequencer id (ForcePlayStageSequencerDevReq / PreLoadSequencerReq + PlayAllPreLoadSequencerAck), (ii) if impossible, show the friend a "watching <name>'s cutscene" hold (letterbox, input frozen, their copy hidden) | RE |
| 4.3 | Skip together: anyone's skip skips for all | todo | find the skip message (capture a skip), relay it; skip in the starter's world only when everyone near pressed it? (user: "skippable with everyone's skip button" = each player's skip button works for all) | 1PC capture |
| 4.4 | During a cutscene, nothing of ours lands (boss safety) | done (boss_cutscene_active) | - | - |

## 5. World sharing
| # | Feature | Status | Approach | Test |
|---|---|---|---|---|
| 5.1 | Time of day sync (host's clock) | partial: time_sync exists, off (B4) | SetFieldTimeReq/Ack | 1PC |
| 5.2 | Weather sync | todo | SyncWeatherStateReq | 1PC capture |
| 5.3 | Crime / wanted: guards react to both | todo | WantedAddCrimeRecordReq, WantedAddWitnessActorReq | 2PC |
| 5.4 | NPC trust / reputation changes | per player | by design (own save) | - |
| 5.5 | Gimmicks: doors, levers, breakable objects, puzzles opened by one open for both | todo | UpdateGimmickReq/Ack, GimmickExecuteFrameEventReq, BreakSceneObjectReq, ApplyPuzzleSavePresetAck; rope/hook tree already relayed | 1PC capture |
| 5.6 | Civilians / NPC schedules shared | todo (large) | NPC loads like enemy copies | RE |
| 5.7 | Map pins and waypoints shared | partial (partner pin) | CreateOrChangePinMarkerReq, CreateOrChangeSelfDestinationReq: share custom waypoints | 1PC |
| 5.8 | Fast travel: guest follows host (Abyss Nexus) | todo | on host loading screen, offer the guest "follow"; teleport via DoTeleportAck/loading | 1PC partly |
| 5.9 | Ocean / water levels, fishing pond state | per player | - | - |

## 6. Life skills, economy, base
| # | Feature | Co-op meaning | Status | Test |
|---|---|---|---|---|
| 6.1 | Fishing side by side (see the other's rod, catch) | (a) | todo (actions relayed already? check) | 1PC |
| 6.2 | Cooking / crafting at camp together | (a) | todo | 1PC |
| 6.3 | Gathering / mining / logging visible to both | (a) | todo | 1PC |
| 6.4 | Trading items/silver between players | (b) new | todo: a give/trade window; inventory messages (ExchangeTradeItemReq, MoveItem*) | RE, 2PC |
| 6.5 | Visit the host's house / camp (housing objects visible) | (a) | todo: SyncHousingPlacedSceneObjectListReq | RE |
| 6.6 | Comrades / dispatch (Management*, Operation*, Research*) | per player | by design | - |
| 6.7 | Farming / ranching (VaryExpandedFarmSlotAck, Ranch*) | per player | by design | - |
| 6.8 | Shared currency rewards from contests and bounties | (b) | todo | 2PC |

## 7. Social and presentation
| # | Feature | Status | Approach | Test |
|---|---|---|---|---|
| 7.1 | Emotes / gestures seen by the other | todo (actions likely relay) | StartActionReq relay covers it if the action exists on the copy | 1PC |
| 7.2 | Photo mode with the friend posed | todo | client-only; copy can be posed by its own emotes | 1PC |
| 7.3 | Text/quick chat on the overlay | todo | Steam overlay chat exists; add quick pings (Ctrl+F9 "come here" etc.) | 1PC |
| 7.4 | Pets / companions (mercenaries) visible to both | todo | mercenary loads like the horse | 1PC |
| 7.5 | Customization changes (CharacterCustomizationAck relayed) | partial (relay fixed 2026-11-01) | verify | 1PC |
| 7.6 | Name tag, map pin, minimap icon | done | - | - |

## 8. Session, party size, robustness
| # | Feature | Status | Approach | Test |
|---|---|---|---|---|
| 8.1 | More than 2 players (scaling is built for 4) | todo | protocol is 1:1 today; star topology on the host | RE, 3PC |
| 8.2 | Death and revive | partial (D26 prompt, checkpoint move best effort) | real loading path for checkpoint | RE |
| 8.3 | Boss-fight deaths (A9) | partial (needs 2PC for "partner wins") | - | 2PC |
| 8.4 | Save safety (backups, warnings, Steam Cloud) | partial | find the 2026-11-01 slot2 writer | 1PC |
| 8.5 | Game update readiness | partial (port_scan, check_opcodes) | - | - |
