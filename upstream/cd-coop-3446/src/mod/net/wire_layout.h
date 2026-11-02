#pragma once
// Co-op+: byte layout of the movement messages the co-op relays, for game 1.0.0.3460.
//
// 1.0.0.3460 reordered the "MoveData" block that MoveActor and StartAction carry and grew it
// by 4 bytes (218 -> 222): the Path (local transform, parent, world transform) now comes
// FIRST, then the transform, the rotation and the position, then the timestamp and a new u32.
// Read from the client's own parser (MoveActorAck vf2 exe+0xC6F2A0 -> MoveData reader
// exe+0x26356B0 -> Path exe+0x148A940) and confirmed on captured 3460 packets (key, position,
// rotation, parent 0xFF..FF on foot, timestamp advancing with real time, StartAction flag
// 1 in 10854 of 11013). RotateActor (176 B) did not change.
//
// Offsets are into the Req payload (after the 5-byte header). The Ack is the same bytes with
// one u8 inserted: MoveActor at kMoveDataEnd, StartAction at kActionMoveData, so every
// StartAction offset from kActionMoveData on is one higher in the Ack (ack_at below).

#include <cstddef>

namespace cdmp::net::wire {

// ---- MoveActorReq: 518 B = key, MoveData (222), tail (12), 10 x 28 history ---------
inline constexpr size_t kMoveReqSize = 518;    // 3446: 514
inline constexpr size_t kMoveLocalPos = 0x04;  // Path.local position (= world on foot); 3446: 0x4C
inline constexpr size_t kMoveLocalRot = 0x10;  // 3446: 0x58
inline constexpr size_t kMoveLocalScale = 0x20;  // 3446: 0x68
inline constexpr size_t kMoveParent = 0x44;    // u64 parent/mount key, all FF on foot; 3446: 0x8C
inline constexpr size_t kMoveWorldPos = 0x4C;  // Path.world (the mount's transform when riding); 3446: 0x94
inline constexpr size_t kMoveWorldRot = 0x58;  // 3446: 0xA0
inline constexpr size_t kMoveWorldBlock = 0x40;  // world transform + two vec3, zeroed on foot
inline constexpr size_t kMoveRot = 0xB4;       // quaternion of the actor; 3446: 0x10
inline constexpr size_t kMovePos = 0xC4;       // world position of the actor; 3446: 0x04
inline constexpr size_t kMoveKeyField = 0xD0;  // u32, sometimes another actor's key; 3446: 0x20
inline constexpr size_t kMoveTs = 0xD4;        // u64 game clock, ms; 3446: 0xD5
inline constexpr size_t kMoveDataEnd = 0xE2;   // the Ack's extra u8 goes here; 3446: 0xDE
inline constexpr size_t kMoveWireSize = 0xEE;  // without the history (what we send); 3446: 0xEA

// ---- StartActionReq: 729 B = key, 2 x 69, MoveData (222) + the rest, u8 flag ----------
inline constexpr size_t kActionReqSize = 729;    // 3446: 725
inline constexpr size_t kActionMoveData = 0x8E;  // unchanged
inline constexpr size_t kActionFlag = 0x2D8;     // the Req's last byte, first byte of B in the Ack; 3446: 0x2D4
// MoveData fields inside StartAction: Move offset - 4 + kActionMoveData.
inline constexpr size_t kActionLocalPos = kActionMoveData + kMoveLocalPos - 4;  // 0x8E; 3446: 0xD6
inline constexpr size_t kActionParent = kActionMoveData + kMoveParent - 4;      // 0xCE; 3446: 0x116
inline constexpr size_t kActionRot = kActionMoveData + kMoveRot - 4;            // 0x13E
inline constexpr size_t kActionPos = kActionMoveData + kMovePos - 4;            // 0x14E; 3446: 0x8E
inline constexpr size_t kActionKeyField = kActionMoveData + kMoveKeyField - 4;  // 0x15A; 3446: 0xAA
inline constexpr size_t kActionTs = kActionMoveData + kMoveTs - 4;              // 0x15E; 3446: 0x15F
inline constexpr size_t kActionAfterMove = kActionMoveData + kMoveDataEnd - 4;  // 0x16C, u32 (a key at times); 3446: 0x168
inline constexpr size_t kActionTarget = kActionAfterMove + 4;                   // 0x170, u32 action target; 3446: 0x16C

// Ack offset of a StartAction Req offset at or past kActionMoveData.
constexpr size_t ack_at(size_t req_offset) { return req_offset + 1; }


// ---- DoFrameEventReq (an attack's frame event) -- measured on 3479 -------------------------
// u8 4 | u32 attacker | ... | FrameEventAccessor at kFrameEventAccessor (u8 type | u8 slot | u32 hash |
// u16 id | u32 frame) | ... | one entry per target at the end, target key first. 2976..3446: accessor at
// 0x1F7, entries of 665 bytes, fixed header 0x203. 3479 capture (95 events, 1-4 targets): the accessor
// moved 4 bytes (hash 0x5CC91323, frame counting 0, 1, 2...), entries are 677 bytes (keys at 647, 1324,
// 2001, 2678 in a 4-target event, the last 677 from the end), the shortest event is 522 bytes.
inline constexpr size_t kFrameEventAccessor = 0x1FB;
inline constexpr size_t kFrameEventTargetEntry = 677;
inline constexpr size_t kFrameEventFixed = 0x207;  // header bytes before the first possible target entry (lower bound)
}  // namespace cdmp::net::wire
