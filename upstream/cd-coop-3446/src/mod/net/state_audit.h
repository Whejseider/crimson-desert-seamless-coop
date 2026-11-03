#pragma once
// Co-op+ (roadmap T08): what a co-op session leaves behind in our game.
//
// One line with every piece of co-op state the mod puts into our world, and whether all of it is back to "no session"
// (no copy of the other player, no seat, no hidden NPC, no enemy copies either way, none of our enemies held back,
// party scaling x1, none of the host's weather stages or cutscene copies running, no duel or hunt). The network loop
// writes it to the log 30 s after a session ends (the host's weather goes after 20 s); a "leak" there is a bug.
// The save itself is never written by Co-op+ (savecheck compares the files after every test session).

#include <string>

namespace cdmp::net {

struct StateAudit {
    std::string line;  // "partner 0 | seat 0 | ..."
    bool clean = true;
    std::string leaks;  // what is not back ("" when clean)
};
StateAudit state_audit();

}  // namespace cdmp::net
