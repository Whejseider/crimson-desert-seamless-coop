# Co-op worlds and shared story (design, 2026-11-04)

User requirements (2026-11-04): like Elden Ring Seamless Co-op; a solo save, plus a save for you and each friend,
for the HOST and the GUEST; the right one loads when you join, and leaving puts you back in your solo world; quest
log and progress update live for everyone; every player's saves protected (never copy over or delete progress).

## 1. Save separation (done, verified)
`src/mod/save_io.*`. The game keeps a slot as a folder `save\<account>\slotN\{lobby.save, save.save}` and reaches
it only through kernelbase file calls (logged with `[dev] save_io_log=1`): it lists `<account>\*`, reads with
CreateFile2, and saves as a folder swap (CreateDirectoryW / CopyFileW / CreateFileW into `slotN_new`, MoveFileExW
`slotN` -> `slotN_backup`, `slotN_new` -> `slotN`, the backup to `..._delete`, DeleteFileW / RemoveDirectoryW).
While a co-op world is active every one of those paths under `save\<account>\` goes to
`%LOCALAPPDATA%\CrimsonDesert-CoopPlus\Worlds\<world>\<account>\`. A new world starts as a copy of the player's slots
(made under `<world>.making`, renamed when complete). Files directly in `save\` (graphics, input) are shared.

Verified: an in-game save with a world active lands in the world; the player's own slot is byte-identical; every
own save equals the backup. (The first attempt showed the game also copies files during a save: an unredirected
CopyFileW made the game roll its own save back. All copy calls are hooked.)

## 2. Switching worlds (built, being tested)
`src/mod/net/coop_worlds.*`, `[coop] coop_worlds=1`. World per partner: `with-steam-<SteamID64>`, for both roles.
- Session up for 8 s -> after a 10 s notice: Esc > Others > Title Screen > confirm; at the title (nothing is being
  saved) the save folder becomes the world; Play (E), retried until the game starts loading. Play loads the newest
  save of the world = where you left off with that friend.
- Partner gone 45 s (a reconnect inside that keeps the world) -> the same back to your own saves.
- Keys/clicks only while the game window is in front; clicks are scaled from the 1920x1080 layout by the window
  height, centred. Every step has a timeout and a plain message.

## 3. The guest's world follows the host's story (next)
When the guest's world with a host is created, and at every join, the guest's WORLD copy gets the host's
QuestSaveData (story sync's transplant, `common/game_save`), written at the title screen between the folder switch
and Play. Never the guest's own saves.

## 4. Live quest progress (`src/mod/net/quest_live.*`, built 2026-11-04)
Quests advance in each game's own in-process server, and only through requests from that game's own client.
- The quest cheat is dead on retail: `CompleteQuestByCheatReq`'s parser (exe+0x2CD4180: u8 x3, then 14-byte
  entries u64/u32/u16 to the end) reads the packet and calls no handler (KillCheatReq's and MissionTriggerInOutReq's
  parsers call theirs). 20 layouts sent, never an answer.
- Mirroring the client requests works: `MissionTriggerInOutReq` (2899: u32 mission, u32 user key, u8 in/out) sent
  into a fresh load (nobody moving) finished "Head to Hernand" and the server moved the quest on (UpdateTraceMissionAck
  2459). The server checks it like its own player's: with a wrong user key, for a step not started yet, or for a
  step of another kind (talk / defeat / interact / read) nothing happens. So mirroring cannot skip ahead.
- quest_live: each mod sends the partner every MissionTriggerInOutReq and QuestDialogEndReq its own client sends;
  the receiver writes its own user key (2360 offset 6) / its own NPC that offers the same dialog (SetQuestDialogAck
  3086 lists each nearby NPC's dialogs) and injects it. Both directions. Only while both players are in a co-op
  world. A step that finishes right after is announced ("Quest step done together with ...").
- Dialogs (talk steps): QuestDialogEndReq = 30 bytes (u32 NPC actor, u32 dialog, u32 x3, u8 x2, u32 x2; handler
  exe+0x2B70290 looks the dialog up in the game data, checks the line index against it, then the NPC). Mapping is
  built; no recording of a real talk step yet, so unproven.
- Defeat steps: KillCheatReq kills work but did not finish anything in two camps (74 + 77 kills, no mission change).
  Why: a cheat kill posts no game event to the quest system at all ([dev] quest_events_log=1, `quest_events.*`), and
  real hits and deaths are worked out by the in-process server, so no client request fakes a real kill.
- Where progress lives (memory diff + hardware watch, 2026-11-04): each mission is a record (u32 key +0x28, u8 state
  +0x2C) upserted by exe+0x1C37B10. The quest advancement routine exe+0x2B95BE0(manager, u32* result, u16* mission
  index, u64* time, u8) queues "this step 5, next step 2" through exe+0x1C3F0F0 and commits them. Calling it on the
  server thread for the partner's finished step is the general route for every step kind (defeat, interact, read):
  it runs the game's own consequences, unlike writing the state byte. Next test: log it during a finished step.
- Still open: defeat steps, interact / read / cutscene steps
  (StageBranchStateReq, UpdateStageChartEventReq... carry per-world stage ids), and a catch-up for anything missed
  (the guest's world gets the host's story at every join already).
