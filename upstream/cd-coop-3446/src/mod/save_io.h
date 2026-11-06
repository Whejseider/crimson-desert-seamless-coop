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
// %LOCALAPPDATA%\CrimsonDesert-CoopPlus\Worlds\<world>\<account>\ instead: the game sees, loads and saves only the
// world's slots. A new world starts as a COPY of the player's own slots (read, never moved or changed). With no
// world active nothing is redirected and the game uses the player's own saves exactly as without the mod. Files
// directly in save\ (graphics and input options) are never redirected.

#include <string>

namespace cdmp::save_io {

// Once, at start, before the title screen reads the saves. log: every save-folder call goes to cdmp.log too.
void install(bool log);

// How a co-op world started (its coopplus-world.ini; the host chooses it the first time, coop_worlds.h).
enum class WorldStart { Unknown, Continue, NewGame };

// The co-op world the game's saves come from now on ("" = the player's own saves). A world that does not exist yet
// is made first as a copy of the player's own slots (start_new_as Continue, or NewGame: a new game is started in it
// at the title, coop_worlds.cpp); Unknown = not made (false). False (nothing changed) when it cannot be prepared; why says so in plain
// words. Switch only while the game is not saving (at the title screen, or right before a load).
bool set_world(const std::wstring& name, std::string* why, WorldStart start_new_as = WorldStart::Continue);
std::wstring world();

bool world_exists(const std::wstring& name);
// The world has a save slot (a new-game world has none until its new game first saves).
bool world_has_saves(const std::wstring& name);
// Unknown: no such world.
WorldStart world_start(const std::wstring& name);
// Co-op+ (2026-11-05): a world made by an earlier version (no coopplus-world.ini: a copy of the player's own progress).
bool world_is_legacy(const std::wstring& name);
void world_mark_continue(const std::wstring& name);
// Renamed to <name>-old-<date>-<time> (kept, never deleted) so the next switch makes a new one. Not while in use.
bool world_archive(const std::wstring& name, std::string* why);
// A new-game world whose new game has not been started yet (the next switch presses New Game, not Play).
bool world_new_game_pending(const std::wstring& name);
void world_mark_new_game_started(const std::wstring& name);

// [dev] coop_world (one-PC tests): this scratch world stands for the player's own saves from now on, so no switch
// of the co-op worlds can reach the real save folder. Must exist (the test scripts make it).
void set_dev_home(const std::wstring& name);
// Two copies on one PC (CDMP_ROLE): the whole save folder, options included, goes to the scratch world. Before
// install / set_dev_home.
void set_whole_folder(bool on);

// Where a world's files live (for backups and the panel).
std::wstring world_folder(const std::wstring& name);

}  // namespace cdmp::save_io
