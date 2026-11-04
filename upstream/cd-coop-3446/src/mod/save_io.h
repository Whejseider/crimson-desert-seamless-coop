#pragma once
// Co-op+ research (per-friend co-op saves): how the game opens its save files.
//
// [dev] save_io_log=1 hooks the Win32 file calls (CreateFileW, CreateFile2, MoveFileExW, ReplaceFileW, DeleteFileW)
// and logs every one whose path is under the game's save folder (...\Pearl Abyss\CD\save\), with the access asked
// for and the game code that called it (exe+rva). Installed at start, before the title screen reads any save.

namespace cdmp::save_io {

void install_log();

}  // namespace cdmp::save_io
