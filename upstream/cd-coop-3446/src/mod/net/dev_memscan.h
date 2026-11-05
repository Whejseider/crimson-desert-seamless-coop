#pragma once
// Co-op+ dev (research only, dev_script "memscan" / "memdiff" / "memwatch"): the memory-editor way of finding where
// the game keeps a value. memscan finds every 4-aligned copy of a byte pattern (e.g. a mission key) in the game's
// read/write memory and keeps the 128 bytes around each; memdiff logs which of those bytes changed since (e.g. a
// mission state 2 -> 5 after the step finished); memwatch puts the hardware write watch (dev_watch.h) on
// match + rel for the nth..nth+3 matches whose bytes there equal a value (every candidate is logged), so the next write names the code that does it.

#include <cstdint>
#include <vector>

namespace cdmp::net {

void dev_memscan(const std::vector<uint8_t>& pattern);
void dev_memdiff();
void dev_memwatch(int rel, const std::vector<uint8_t>& value, int len, int hits, int nth);

// Float narrowing (e.g. the game's speed: 1.0 normally, lower during a slow motion). fscan collects every 4-aligned
// float equal to `value` in read/write memory (heap and the exe's own data); ffilter keeps those now within
// [lo, hi] and logs them once few are left (with exe+offset for the exe's data); fwatch puts the hardware write
// watch on the first four left.
void dev_fscan(float value);
// The same for every float within [lo, hi] (e.g. a frame time step).
void dev_fscan_range(float lo, float hi);
void dev_ffilter(float lo, float hi);
void dev_fwatch(int hits);
// The same for 8-byte doubles (8-aligned): dscan / dfilter / dwatch.
void dev_dscan(double lo, double hi);
void dev_dfilter(double lo, double hi);
void dev_dwatch(int hits);

}  // namespace cdmp::net
