#pragma once
// Co-op+ dev: two copies of the game on one PC (tests only; docs/research/two-copies-oldbase.md). The environment
// variable CDMP_ROLE=host|guest (native_layer.h test_role) gives each copy its own files (cdmp.<role>.ini / .log), its
// own saves (the whole save folder goes to its [dev] coop_world, which must be set) and keeps the copy without the
// focus at full speed. Players never set CDMP_ROLE: nothing here runs for them.

#include <string>

namespace cdmp::two_copy {

// The game starts a short-lived second CrimsonDesert.exe ~40 s after launch, which inherits CDMP_ROLE. True in such a
// process (another process of this role runs already): the mod stays out of it, before even opening the log.
bool extra_process(const std::wstring& role);

// The copy without the focus fell to 2-4 fps (ByJunter's two-copy notes: its GPU work waits behind the focused
// copy's). While another window has the focus, this process's GPU scheduling class goes up to High (AboveNormal if
// refused), and Windows' power throttling (EcoQoS, the timer-resolution clamp) is turned off for it.
void keep_full_speed();

}  // namespace cdmp::two_copy
