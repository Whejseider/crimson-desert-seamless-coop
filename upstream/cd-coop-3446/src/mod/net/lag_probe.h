#pragma once
// Co-op+ dev ([dev] lag_probe=1): measures how far behind the other player's copy is drawn. Every 4 ms a thread
// reads where our own character and the copy are drawn (game memory) and writes them with a microsecond clock to
// bin64/cdmp_lag.csv. With the echo peer the copy follows our own moves, so the time shift that best lines the two
// paths up (tools/lag/lag.py) is the whole delay from our screen to the copy: capture, network, the playout buffer and
// the game's own interpolation. Pipeline marks (a Move sent, the echo received, injected) go to the same file.

#include <cstdint>

namespace cdmp::net {

void lag_probe_start();
// A mark in the file: kind 1 = one of our Moves sent, 2 = a Move of the other player received, 3 = it was injected.
// `ts` is the Move's own timestamp (sender clock), so the three marks of one Move line up. Any thread.
void lag_probe_mark(int kind, uint64_t ts);
bool lag_probe_on();

}  // namespace cdmp::net
