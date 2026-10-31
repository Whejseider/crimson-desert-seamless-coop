#pragma once
// Co-op+ dev diagnostic ([dev] load_trace=1): logs who advances an actor's load state.
//
// An actor's u16 [actor+0x5E] goes 1 -> 2 -> 4 -> 8 -> 0x10 through tiny virtual methods; the
// client holds network movement for an actor until it reaches 0x10. On 3460 the other player's
// copy stops at 8. These hooks log, per actor key, which code calls the 4 -> 8 and 8 -> 0x10
// steps, so a real actor's path can be compared with the copy's.

namespace cdmp::net {

void load_trace_enable(bool on);
// 1 = trace, 2 = trace + clear the copy's hide flag (status +0x260) and set its alpha (experiment).
void load_trace_set_mode(int mode);
int load_trace_mode();
// Late init (before a save loads, so the local player's own load is traced too).
void load_trace_install();

}  // namespace cdmp::net
