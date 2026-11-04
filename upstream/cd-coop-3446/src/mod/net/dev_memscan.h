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

}  // namespace cdmp::net
