#pragma once
// Addresses for CrimsonDesert.exe 1.0.0.3460 (Co-op+ port, 2026-10-31).
//
// Steam updated the game from 1.0.0.3446 (PE 0x6AE46DD0, 332,786,072 bytes) to 1.0.0.3460
// (PE 0x6AE58024, 329,084,312 bytes) on 2026-10-31 at 08:54. The 3446 executable was replaced
// before a copy was kept, so this port could not diff the two builds. Every value below was
// found in the 3460 image by an identity that does not depend on the old bytes:
//
//   * data globals -- the same upstream signatures verify-port decodes (one match each);
//   * vtables -- by RTTI class name (`cdmp_probe --image ... vtables`), one match each;
//   * movement-method Enters -- slot +0x38 of their class's vtable, head checked;
//   * code with a long head or context -- the recorded bytes, one match each;
//   * the two action-layer sites -- the 64-byte context recorded in offsets_3446.h: the first
//     call in it has the SAME rel32 (E8 6F 68 02 00) in 3424, 3446 and 3460, which singles out
//     one of the ~230 instances of that template; the pending site keeps its 0x670 gap from it.
//
// Shifts 3446 -> 3460 (not one constant, as on every patch):
//   code  +0x480 (packet hooks) .. +0x9A0 (movement) .. +0xB40 (CC) .. +0x3370/+0x3730 (layers)
//   data  WorldSystem/ChunkOrigin/minimap global +0x4260, StaticPos +0x4220, Camera/clock +0x4108
// scripts/verify-port.ps1 re-derives each of these from the image.

#include "mod/game/offsets_3446.h"

namespace cdmp::game::build_3460 {

// Everything a patch does not move (opcodes, struct offsets, heads that still match).
using build_3446::RelayedAck;
using build_3446::kActionLayerAdvanceHead;
using build_3446::kActionLayersAdvanceHead;
using build_3446::kActorManager_ControlledActor;
using build_3446::kActor_ComponentTable;
using build_3446::kActor_Key;
using build_3446::kActor_OwnerUser;
using build_3446::kCameraBlockSentinelValue;
using build_3446::kCameraBlock_Forward;
using build_3446::kCameraBlock_Position;
using build_3446::kCameraBlock_Projection;
using build_3446::kCameraBlock_Sentinel;
using build_3446::kCameraBlock_View;
using build_3446::kCamera_Block;
using build_3446::kClientClock;
using build_3446::kClientClockHead;
using build_3446::kClientOnRecvPacketHead;
using build_3446::kClientPostPacketHead;
using build_3446::kComponents_CharacterControl;
using build_3446::kComponents_Status;
using build_3446::kControlInner_Physics;
using build_3446::kControl_Inner;
using build_3446::kControl_Layer0;
using build_3446::kControl_MoveController;
using build_3446::kControl_WallData;
using build_3446::kCoopPinId;
using build_3446::kFieldTimeClient;
using build_3446::kHookArgSizeStackOffset;
using build_3446::kLayer_Chart;
using build_3446::kLayer_Playback;
using build_3446::kLoadKindMount;
using build_3446::kLoadPositionFromEnd;
using build_3446::kLoadRotationFromEnd;
using build_3446::kLoadingParkHeight;
using build_3446::kLoadingParkRadius;
using build_3446::kMaxStatEntries;
using build_3446::kMethod_Actor;
using build_3446::kMinimapIconCompanion;
using build_3446::kMinimapIconOutlinedArrow;
using build_3446::kMinimapIconPlayerArrow;
using build_3446::kMoveController_Method;
using build_3446::kMoveController_WallUp;
using build_3446::kMoveMethodWallUp;
using build_3446::kOpActorLoadDataAck;
using build_3446::kOpChangeDockingSocketByFrameEventAck;
using build_3446::kOpCreateOrChangePinMarkerAck;
using build_3446::kOpMoveActorAck;
using build_3446::kOpMoveActorReq;
using build_3446::kOpOtherEquipItemAck;
using build_3446::kOpOtherUnequipItemAck;
using build_3446::kOpRemoteCatchResultAck;
using build_3446::kOpRemoveActorAck;
using build_3446::kOpRemovePinMarkerAck;
using build_3446::kOpRotateActorAck;
using build_3446::kOpRotateActorReq;
using build_3446::kOpSetFieldTimeAck;
using build_3446::kOpSetFieldTimeReq;
using build_3446::kOpStartActionAck;
using build_3446::kOpStartActionReq;
using build_3446::kOpUnequipItemAck;
using build_3446::kOpUpdateOnDemandGimmickAck;
using build_3446::kOpUpdateWallUpDataAck;
using build_3446::kPhysics_AxisX;
using build_3446::kPhysics_AxisY;
using build_3446::kPhysics_AxisZ;
using build_3446::kPhysics_Position;
using build_3446::kPinColourCount;
using build_3446::kPinGroupNormal;
using build_3446::kPinShapeCount;
using build_3446::kPointClimbEnterHead;
using build_3446::kRelayedAcks;
using build_3446::kRemoteCatchSceneId;
using build_3446::kRideEnterHead;
using build_3446::kServerActor_Key;
using build_3446::kServerDockingWriterHead;
using build_3446::kServerOtherEquipWriterHead;
using build_3446::kStatContainer_Count;
using build_3446::kStatContainer_Entries;
using build_3446::kStatEntry_Current;
using build_3446::kStatEntry_Id;
using build_3446::kStatEntry_Max;
using build_3446::kStatEntry_Size;
using build_3446::kStatId_Health;
using build_3446::kStatId_Stamina;
using build_3446::kStatus_StatContainer;
using build_3446::kTroc_Kind;
using build_3446::kTroc_Opcode;
using build_3446::kVtCamera;
using build_3446::kVtPhysicsObject;
using build_3446::kWorldSystem_ActorManager;

// ---- build identity -------------------------------------------------------
inline constexpr uint32_t kPeTimestamp = 0x6AE58024;  // 3446:0x6AE46DD0
inline constexpr uintptr_t kImageBase = 0x140000000;  // fixed, no ASLR (restated: verify-port reads it from here)
inline constexpr uintptr_t kImageSize = 0x13EE0000;   // 3446:0x14268000

// ---- static globals (RVAs, zero-filled in the file) -------------------------
// Each decoded from its signature site, which moved too (sites in brackets).
inline constexpr uintptr_t kWorldSystemSlot = 0x71B5CA0;  // 3446:0x71B1A40 [world_system_ref 0xC9C6DB, _p3 0x3AAEC6]
inline constexpr uintptr_t kCameraSlot = 0x70D1FB0;       // 3446:0x70CDEA8 [camera_global_ref 0x2FBC4E7]
inline constexpr uintptr_t kStaticPlayerPos = 0x71C4E40;  // 3446:0x71C0C20 [player_static_pos_write 0xA3014A]
inline constexpr uintptr_t kChunkOrigin = 0x71A1F00;      // 3446:0x719DCA0 [world_offset_sub 0x3977221]

// The clock flag: 3446 0x70CDCAD + the camera region's +0x4108. 18 `cmp byte [rip+d],0` sites
// in the 3460 code land on it, and code reaches +0x34 (5 refs) and +0x3C (15 refs). Live at the
// title screen: flag = 1, +0x3C = 33799. The fields are checked for plausibility at runtime
// (game_clock.cpp), so a wrong slot only turns the game clock off.
inline constexpr uintptr_t kClientClockGlobal = 0x70D1DB5;   // 3446:0x70CDCAD
inline constexpr uintptr_t kClientFrameClockMs = 0x70D1DE9;  // = flag + 0x34
inline constexpr uintptr_t kClientFrameClockUs = 0x70D1DF1;  // = flag + 0x3C
// Same struct-relative gaps from kWorldSystemSlot as on 2976/3424/3446; code reaches all three.
inline constexpr uintptr_t kClientClockBias = 0x71B5F48;     // = kWorldSystemSlot + 0x2A8
inline constexpr uintptr_t kMinimapIconRoot = 0x71B6038;     // = kWorldSystemSlot + 0x398
inline constexpr uintptr_t kMinimapIconRootAlt = 0x71B6040;  // = kWorldSystemSlot + 0x3A0

// ---- vtables, by RTTI name -------------------------------------------------
inline constexpr uintptr_t kVtClientActorManager = 0x593DCF0;    // .?AVClientActorManager@pa@@ (3446:0x5939CD8)
inline constexpr uintptr_t kVtControlledActor = 0x590BCA0;       // .?AVClientChildOnlyInGameActor@pa@@ (3446:0x5907C90)
inline constexpr uintptr_t kVtCharacterControl = 0x593C8E0;      // .?AVClientCharacterControlActorComponent@pa@@
inline constexpr uintptr_t kVtStatusComponent = 0x59136E0;       // .?AVClientStatusActorComponent@pa@@
inline constexpr uintptr_t kVtWallUpMovementMethod = 0x591CF78;  // .?AVWallUpMovementMethod@pa@@

// ---- movement-method Enter hooks: slot +0x38 of the class vtable -----------------
// PointClimb vtable 0x591D6A8, RideOn 0x591D868, RideOff 0x591D758. Each Enter moved by the
// same +0x9A0, and each opens with the head offsets_3446.h records.
inline constexpr uintptr_t kPointClimbEnter = 0x75FFC0;  // 3446:0x75F620
inline constexpr uintptr_t kRideOnEnter = 0x74F270;      // 3446:0x74E8D0
inline constexpr uintptr_t kRideOffEnter = 0x750240;     // 3446:0x74F8A0

// ---- action layers ---------------------------------------------------------------
// kActionLayerAdvance: its 16-byte head (a float-delta advance) occurs once.
inline constexpr uintptr_t kActionLayerAdvance = 0x214C0C0;  // 3446:0x2148D50
// kActionLayersAdvance: the head is shared by ~230 template instances; the context's first
// call keeps rel32 0x2686F in every build, which matches once, at 0x232DFC9 (site +0x17).
inline constexpr uintptr_t kActionLayersAdvance = 0x232DFE0;  // 3446:0x232A8B0

// ---- minimap icon decision (mid-hook) -------------------------------------------
// The interior sequence still occurs once; the site is +0x0A. Its `mov eax,[rip+d]` now loads
// 0x7140A30 (3446 0x713C7D0, the same +0x4260 as WorldSystem), so the head is re-recorded.
inline constexpr uintptr_t kMinimapIconDecide = 0x9E95CD;  // 3446:0x9E8A7D
inline constexpr uint8_t kMinimapIconDecideHead[] = {0x80, 0x7C, 0x24, 0x48, 0x00, 0x75, 0x6F, 0x8B,
                                                     0x05, 0x56, 0x74, 0x75, 0x06, 0x41, 0x89, 0x06};

// ---- server-side writer of OtherEquipItemAck: NOT PORTED ------------------------
// The 16 site bytes and the entry prologue are shared by ~40 item-list writers in 3460, and
// the 3446 executable that would tell them apart is gone. 0 makes the head check fail, so
// live gear changes are not relayed (gear at spawn still comes from the actor templates).
inline constexpr uintptr_t kServerOtherEquipWriter = 0;  // 3446:0x29EDD11
// The docking relay is compiled out (local_capture.cpp, kDockingRelay); never ported past 3424.
inline constexpr uintptr_t kServerDockingWriter = 0;

// ---- packet hooks: their long upstream signatures (troc-dispatch.md) match once ---------
inline constexpr uintptr_t kClientPostPacket = 0x3A5A30;    // 3446:0x3A55B0
inline constexpr uintptr_t kClientOnRecvPacket = 0x3A5DC0;  // 3446:0x3A5940

// ---- Troc handler instances (runtime objects; measured live) -------------------
// probe troc exe+6D00000 exe+6FF0000 on the running 3460 game: 1175 handlers, first 0x6E86D10,
// last 0x6EBA370, span 0x33660 -- the 3446 range moved by exactly +0x4000, same count, same span.
// The mod's own walk then resolved the same 1175 types.
inline constexpr uintptr_t kTrocInstancesBegin = 0x6E86D10;  // 3446:0x6E82D10
inline constexpr uintptr_t kTrocInstancesEnd = 0x6EBA378;    // last + 8 (3446:0x6EB6378)

}  // namespace cdmp::game::build_3460
