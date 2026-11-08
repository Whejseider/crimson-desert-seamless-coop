#pragma once
// Co-op+ on the native base (docs/design/native-join-and-saves.md): the native base's launch mode (Solo / Host /
// Guest) lives in its cdmp.ini (net/native_mode_text.h has the text). The layer writes it when the host starts co-op
// at the title, restarts the game into that mode, and writes Solo again once the session is over, so a normal launch
// is always the player's own game.

#include <cstdint>
#include <string>

#include "mod/net/native_mode_text.h"

namespace cdmp::net {

// The native ini this process's native base reads: bin64\cdmp.ini (cdmp.<role>.ini with CDMP_ROLE, two copies).
std::wstring native_mode_ini_path(const std::wstring& bin_dir);

// The mode that ini is in now (Solo when absent or not written by Co-op+).
NativeMode native_mode_current(const std::wstring& bin_dir);

// Writes the ini for `spec` (UTF-16, as the native base's GetPrivateProfileString reads it). A cdmp.ini that Co-op+
// did not write is kept once as cdmp.ini.before-coopplus. False (logged) if it could not be written.
bool native_mode_write(const std::wstring& bin_dir, const NativeModeSpec& spec);

// Restarts the game: a hidden helper waits for this process to end and starts the game again (through Steam; with
// CDMP_ROLE, the exe itself with this process's environment, as the two-copy harness starts it), then this process
// ends. Only at the title (nothing to save). Does not return when it works; false (logged) if the helper failed.
bool native_restart_game(const char* why);

}  // namespace cdmp::net
