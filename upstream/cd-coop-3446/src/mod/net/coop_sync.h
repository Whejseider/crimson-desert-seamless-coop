#pragma once
// Co-op+ (roadmap T06): desync detection and resync.
//
// Every kDigestMs each side sends a digest (Activity type 15): how many enemy copies it sent the other player and how
// many copies of the other player's enemies it has in its world right now (loaded, or waiting to load). When the
// other side's "sent" and our "present" disagree on kMismatches digests in a row, we ask for a resync: the session is
// dropped and joined again (the reconnection path verified in T04: every copy removed, sent again once). The other
// player's character drawn far from where they say they are is reloaded locally (coop.cpp). Time and weather are
// resent all the time by their own modules. A manual resync is the Resync action (Shift+F8 / View + RB).

#include <cstddef>
#include <cstdint>
#include <string>

namespace cdmp::net {

// Network loop, every tick.
void sync_tick(bool connected, uint32_t now_ms);

// Activity type 15 from the other player.
void sync_on_event(const uint8_t* data, size_t n);

// A resync the digests asked for ("" = none): the reason, once.
std::string sync_take_resync();

// [dev] desync_test: 30 s after each connection, 1 = one copy of the other player's enemies is removed from our
// world behind the relay's back and the other player's character stops following them for 25 s; 2 = only the
// character, frozen for 60 s (the drift check).
void sync_set_test(int mode);

}  // namespace cdmp::net
