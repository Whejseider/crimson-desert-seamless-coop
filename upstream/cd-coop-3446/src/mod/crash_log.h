#pragma once
// Co-op+: writes fatal-looking exceptions and process exit to cdmp.log, so a game that
// closes on its own leaves a trace (the game's own crash reporter writes nothing for us).

namespace cdmp::crash_log {

// Installs a vectored handler that only logs (it never handles anything). First-chance
// exceptions the mod's own guarded reads raise on purpose are logged too, so each faulting
// address is logged at most 3 times; the LAST lines before a crash are the ones that matter.
void install();

// From DllMain(DLL_PROCESS_DETACH): logs whether the process is exiting normally.
void note_detach(bool process_exit);

}  // namespace cdmp::crash_log
