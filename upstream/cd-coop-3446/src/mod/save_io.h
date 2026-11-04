#pragma once
// Co-op+ co-op worlds: the game's save files, separated per co-op world (like Elden Ring Seamless Co-op's own save
// file), so co-op play can never change a player's own (solo) saves.
//
// The game keeps each slot as a folder, %LOCALAPPDATA%\Pearl Abyss\CD\save\<account>\slotN\{lobby.save, save.save}.
// It lists the account folder (FindFirstFileW <account>\*), reads with CreateFile2, and writes a slot as a folder
// swap: CreateDirectoryW / CreateFileW slotN_new\..., MoveFileExW slotN -> slotN_backup, slotN_new -> slotN,
// slotN_backup -> slotN_backup_<n>_delete, DeleteFileW / RemoveDirectoryW (logged with [dev] save_io_log=1,
// 2026-11-04). Every one of those calls goes through kernelbase, where this module hooks them.
//
// While a co-op world is active, every path under save\<account>\ is sent to
// Documents\CrimsonDesert-CoopPlus\Worlds\<world>\<account>\ instead: the game sees, loads and saves only the
// world's slots. A new world starts as a COPY of the player's own slots (read, never moved or changed). With no
// world active nothing is redirected and the game uses the player's own saves exactly as without the mod. Files
// directly in save\ (graphics and input options) are never redirected.

#include <string>

namespace cdmp::save_io {

// Once, at start, before the title screen reads the saves. log: every save-folder call goes to cdmp.log too.
void install(bool log);

// The co-op world the game's saves come from now on ("" = the player's own saves). A world that does not exist yet
// is made as a copy of the player's own slots first. False (nothing changed) when it cannot be prepared; why says
// so in plain words. Switch only while the game is not saving (at the title screen, or right before a load).
bool set_world(const std::wstring& name, std::string* why);
std::wstring world();

// Where a world's files live (for backups and the panel).
std::wstring world_folder(const std::wstring& name);

}  // namespace cdmp::save_io
