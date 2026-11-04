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

## 4. Live quest progress (research)
Quests advance in each game's own in-process server. Routes, in order of preference:
- Mirror the host's mission changes (UpdateMissionListAck, observed by the host's mod) to the guest's server with
  `CompleteQuestByCheatReq` (3041, 17 bytes: u8, u8, u8, u64, u32, u16). Other cheat requests (KillCheatReq) are
  honoured on retail; this one was thought ignored (D42) but its layout was never known. Live test next.
- Mirror the client requests that drive quests (MissionTriggerInOutReq [u32, u32, u8], QuestDialogEndReq ...).
- Both directions (a guest's step counts for the host too), only for quests both have under way, never out of order.
