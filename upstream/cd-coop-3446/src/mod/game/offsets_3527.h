#pragma once
// Addresses for CrimsonDesert.exe 1.0.0.3527 (Co-op+ port, 2026-11-04).
//
// Steam updated the game from 1.0.0.3479 (PE 0x6AE6E3E8) to 1.0.0.3527 (PE 0x6AEA07A6, 328,899,480 bytes) on
// 2026-11-04. With both exes on disk (build/exe-archive) every value was mapped by build/port3527/portmap.py: code
// by its instructions with relative operands masked (one match each, nearest kept), data globals and vtables
// through the code that addresses them (references agree), vtables checked against RTTI; then checked live.
//
// Shifts 3479 -> 3527 vary by region: code +0x600 (packet hooks) .. +0xDF0 (character control) .. +0x1510
// (error logger, load steps) .. +0x33B0 (action layers, pending); vtables +0x5B28..+0x6030; data +0x7000 ..
// +0x72C0.

#include "mod/game/offsets_3479.h"

namespace cdmp::game::build_3527 {

// Everything this patch did not move (opcodes, struct offsets, wire layout, heads that match).
using build_3479::RelayedAck;
using build_3479::kActionLayerAdvanceHead;
using build_3479::kActionLayersAdvanceHead;
using build_3479::kActorManager_ControlledActor;
using build_3479::kActor_ComponentTable;
using build_3479::kActor_Key;
using build_3479::kActor_OwnerUser;
using build_3479::kCameraBlockSentinelValue;
using build_3479::kCameraBlock_Forward;
using build_3479::kCameraBlock_Position;
using build_3479::kCameraBlock_Projection;
using build_3479::kCameraBlock_Sentinel;
using build_3479::kCameraBlock_View;
using build_3479::kClientClock;
using build_3479::kClientClockHead;
using build_3479::kClientOnRecvPacketHead;
using build_3479::kClientPostPacketHead;
using build_3479::kComponents_CharacterControl;
using build_3479::kComponents_Status;
using build_3479::kControlInner_Physics;
using build_3479::kControl_Inner;
using build_3479::kControl_Layer0;
using build_3479::kControl_MoveController;
using build_3479::kControl_WallData;
using build_3479::kCoopPinId;
using build_3479::kHookArgSizeStackOffset;
using build_3479::kLayer_Chart;
using build_3479::kLayer_Playback;
using build_3479::kLoadKindMount;
using build_3479::kLoadingParkHeight;
using build_3479::kLoadingParkRadius;
using build_3479::kMaxStatEntries;
using build_3479::kMethod_Actor;
using build_3479::kMinimapIconCompanion;
using build_3479::kMinimapIconOutlinedArrow;
using build_3479::kMinimapIconPlayerArrow;
using build_3479::kMoveController_Method;
using build_3479::kMoveController_WallUp;
using build_3479::kMoveMethodWallUp;
using build_3479::kPhysics_AxisX;
using build_3479::kPhysics_AxisY;
using build_3479::kPhysics_AxisZ;
using build_3479::kPhysics_Position;
using build_3479::kPinColourCount;
using build_3479::kPinGroupNormal;
using build_3479::kPinShapeCount;
using build_3479::kPointClimbEnterHead;
using build_3479::kRelayedAcks;
using build_3479::kRemoteCatchSceneId;
using build_3479::kRideEnterHead;
using build_3479::kServerActor_Key;
using build_3479::kServerDockingWriterHead;
using build_3479::kServerOtherEquipWriterHead;
using build_3479::kStatContainer_Count;
using build_3479::kStatContainer_Entries;
using build_3479::kStatEntry_Current;
using build_3479::kStatEntry_Id;
using build_3479::kStatEntry_Max;
using build_3479::kStatEntry_Size;
using build_3479::kStatId_Health;
using build_3479::kStatId_Stamina;
using build_3479::kStatus_StatContainer;
using build_3479::kTroc_Kind;
using build_3479::kTroc_Opcode;
using build_3479::kVtCamera;
using build_3479::kVtPhysicsObject;
using build_3479::kWorldSystem_ActorManager;
using build_3479::kCamera_Block;
using build_3479::kLoadPositionFromEnd;
using build_3479::kLoadRotationFromEnd;
using build_3479::kOpSetFieldTimeReq;
using build_3479::kOpSetFieldTimeAck;
using build_3479::kOpActorLoadDataAck;
using build_3479::kOpRemoveActorAck;
using build_3479::kOpMoveActorReq;
using build_3479::kOpMoveActorAck;
using build_3479::kOpStartActionReq;
using build_3479::kOpStartActionAck;
using build_3479::kOpRotateActorReq;
using build_3479::kOpRotateActorAck;
using build_3479::kOpUpdateWallUpDataAck;
using build_3479::kOpUnequipItemAck;
using build_3479::kOpOtherEquipItemAck;
using build_3479::kOpOtherUnequipItemAck;
using build_3479::kOpCreateOrChangePinMarkerAck;
using build_3479::kOpRemovePinMarkerAck;
using build_3479::kOpChangeDockingSocketByFrameEventAck;
using build_3479::kOpRemoteCatchResultAck;
using build_3479::kOpUpdateOnDemandGimmickAck;
using build_3479::kStatus_HideFlags;
using build_3479::kHideFlagsOfCopies;
using build_3479::kStatusFlagsChangedHead;
using build_3479::kMovementMethodStepHead;
using build_3479::kServerOtherEquipWriter;
using build_3479::kComponents_EquipSlot;
using build_3479::kEquipSlot_Serial;
using build_3479::kEquipSlot_Table;
using build_3479::kSlotTable_Entries;
using build_3479::kSlotTable_Count;
using build_3479::kSlotEntry_Size;
using build_3479::kSlotEntry_ItemIndex;
using build_3479::kSlotEntry_Count;
using build_3479::kSlotEntry_ShownIndex;
using build_3479::kSlotEntry_Slot;
using build_3479::kItemInfoTable_Count;
using build_3479::kItemInfoTable_Records;
using build_3479::kItemInfo_Key;
using build_3479::kServerDockingWriter;
using build_3479::kMiniGameRoot_Holder;
using build_3479::kMiniGameHolder_Gauge;
using build_3479::kErrorLoggerHead;

// ---- moved in 3527 ----------------------------------------------------------
inline constexpr uint32_t kPeTimestamp = 0x6AEA07A6;  // 3479:0x6AE6E3E8
inline constexpr uintptr_t kImageBase = 0x140000000;  // fixed, no ASLR
inline constexpr uintptr_t kImageSize = 0x13EB2000;  // 3479:0x14073000
inline constexpr uintptr_t kWorldSystemSlot = 0x71BDEF0;  // 3479:0x71B6C50, 8/8 code references agree
inline constexpr uintptr_t kCameraSlot = 0x70DA028;  // 3479:0x70D2F68
inline constexpr uintptr_t kStaticPlayerPos = 0x71CD0B0;  // 3479:0x71C5DF0
inline constexpr uintptr_t kChunkOrigin = 0x71AA150;  // 3479:0x71A2EB0
inline constexpr uintptr_t kCutscenePlaying = 0x70D9F14;  // 3479:0x70D2E2F, 3/3 references
inline constexpr uintptr_t kFieldTimeClient = 0x6EAB998;  // 3479:0x6EA4998, 6/6 references
inline constexpr uintptr_t kMiniGameRoot = 0x71BDF48;  // 3479:0x71B6CA8
inline constexpr uintptr_t kClientClockGlobal = 0x70D9E3D;  // 3479:0x70D2D75, 8/8 cmp sites
inline constexpr uintptr_t kClientFrameClockMs = 0x70D9E71;  // = flag + 0x34
inline constexpr uintptr_t kClientFrameClockUs = 0x70D9E79;  // = flag + 0x3C
inline constexpr uintptr_t kClientClockBias = 0x71BE198;  // = kWorldSystemSlot + 0x2A8 (references agree)
inline constexpr uintptr_t kMinimapIconRoot = 0x71BE288;  // = kWorldSystemSlot + 0x398
inline constexpr uintptr_t kMinimapIconRootAlt = 0x71BE290;  // = kWorldSystemSlot + 0x3A0
inline constexpr uintptr_t kStatusFlagsChanged = 0x6E99D0;  // 3479:0x6E90A0, head matches
inline constexpr uintptr_t kVtNormalMovementMethod = 0x5958C10;  // 3479:0x5952BF8; slot 2 = kMovementMethodStep
inline constexpr uintptr_t kMovementMethodStep = 0xBBD5F0;  // 3479:0xBBC570, head matches
inline constexpr uintptr_t kVtClientActorManager = 0x5944B38;  // 3479:0x593ECF0 (= RTTI)
inline constexpr uintptr_t kVtControlledActor = 0x59127C8;  // 3479:0x590CCA0 (= RTTI)
inline constexpr uintptr_t kVtCharacterControl = 0x59438F0;  // 3479:0x593D8E0 (= RTTI)
inline constexpr uintptr_t kVtStatusComponent = 0x591A710;  // 3479:0x59146E0 (= RTTI)
inline constexpr uintptr_t kVtWallUpMovementMethod = 0x5923FA8;  // 3479:0x591DF78 (= RTTI)
inline constexpr uintptr_t kActionLayerAdvance = 0x214EC90;  // 3479:0x214C070, head matches
inline constexpr uintptr_t kActionLayersAdvance = 0x2331320;  // 3479:0x232DF70; one of many identical template copies: found through its caller, keeps the 0x670 gap to kPending
inline constexpr uintptr_t kMinimapIconDecide = 0x9EA31D;  // 3479:0x9E959D
inline constexpr uintptr_t kVtClientEquipSlotComponent = 0x594B388;  // 3479:0x5945388
inline constexpr uintptr_t kItemInfoTableSlot = 0x71BEE70;  // 3479:0x71B7BD0, decoded from the load below
inline constexpr uintptr_t kItemInfoTableLoad = 0x3DD5D5;  // 3479:0x3DCFD5
inline constexpr uintptr_t kTrocInstancesBegin = 0x6E8ED10;  // 3479:0x6E87D10, same span
inline constexpr uintptr_t kTrocInstancesEnd = 0x6EC2378;  // last + 8
inline constexpr uintptr_t kGameplayInput = 0x719F558;  // 3479:0x71982B8, u8
inline constexpr uintptr_t kUiModePush = 0xF21B07;  // 3479:0xF20AC7; its movzx reads kGameplayInput
inline constexpr uintptr_t kErrorLogger = 0x15FC450;  // 3479:0x15FAF40, head matches
inline constexpr uintptr_t kClientPostPacket = 0x3A6030;  // 3479:0x3A5A30, head matches
inline constexpr uintptr_t kClientOnRecvPacket = 0x3A63C0;  // 3479:0x3A5DC0, head matches
inline constexpr uintptr_t kPointClimbEnter = 0x760A10;  // 3479:0x75FFC0, head matches
inline constexpr uintptr_t kRideOnEnter = 0x74FBB0;  // 3479:0x74F270, head matches
inline constexpr uintptr_t kRideOffEnter = 0x750B80;  // 3479:0x750240, head matches
inline constexpr uint8_t kMinimapIconDecideHead[] = {0x80, 0x7C, 0x24, 0x48, 0x00, 0x75, 0x6F, 0x8B, 0x05, 0x56, 0xE9, 0x75, 0x06, 0x41, 0x89, 0x06};  // its mov eax,[rip+d] moved with the data

}  // namespace cdmp::game::build_3527
