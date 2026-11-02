#pragma once
// Co-op+ dev (research only, dev_script "watch"): a hardware data breakpoint. Debug register 0 of every game thread is
// set to fire on writes to `addr` (1, 2, 4 or 8 bytes); a vectored handler records who wrote (RIP as exe+offset, the
// general registers, xmm0/xmm1) for the first `hits` writes, then the watch removes itself and the records go to
// cdmp.log. Used to find where a value on screen comes from (minigame gauge, weather, cutscenes).

#include <cstdint>

namespace cdmp::net {

void dev_watch_start(uintptr_t addr, int len, int hits);

}  // namespace cdmp::net
