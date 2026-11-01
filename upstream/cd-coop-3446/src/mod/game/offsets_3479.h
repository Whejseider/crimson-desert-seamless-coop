#pragma once
// Addresses for CrimsonDesert.exe 1.0.0.3479 (game version 2.04.05; Co-op+ port, 2026-11-01).
//
// Steam updated the game from 1.0.0.3460 (PE 0x6AE58024) to 1.0.0.3479 (PE 0x6AE6E3E8,
// 330,738,072 bytes) on 2026-11-01. A small patch: every value below was re-found in the 3479
// image (build/exe-archive keeps a copy from now on) with the same identities as the 3460 port:
// signatures for data globals, RTTI names for vtables, recorded heads for code, and a live
// read-only probe for the Troc table, whose 1175 opcodes are identical to 3460's.
//
// Shifts 3460 -> 3479: code +0 (packet hooks, climb/ride, status handler, CC post) .. -0x30
// (CC, movement step) .. -0x50 (layer advance, load-state steps) .. -0x70 (layers, pending);
// vtables and the Troc table +0x1000; data globals +0xFB0 (camera +0xFB8, clock +0xFC0).

#include "mod/game/offsets_3460.h"

namespace cdmp::game::build_3479 {

// Everything this patch did not move (opcodes, struct offsets, wire layout, heads that match).
using build_3460::RelayedAck;
using build_3460::kActionLayerAdvanceHead;
using build_3460::kActionLayersAdvanceHead;
using build_3460::kActorManager_ControlledActor;
using build_3460::kActor_ComponentTable;
using build_3460::kActor_Key;
using build_3460::kActor_OwnerUser;
using build_3460::kCameraBlockSentinelValue;
using build_3460::kCameraBlock_Forward;
using build_3460::kCameraBlock_Position;
using build_3460::kCameraBlock_Projection;
using build_3460::kCameraBlock_Sentinel;
using build_3460::kCameraBlock_View;
using build_3460::kClientClock;
using build_3460::kClientClockHead;
using build_3460::kClientOnRecvPacketHead;
using build_3460::kClientPostPacketHead;
using build_3460::kComponents_CharacterControl;
using build_3460::kComponents_Status;
using build_3460::kControlInner_Physics;
using build_3460::kControl_Inner;
using build_3460::kControl_Layer0;
using build_3460::kControl_MoveController;
using build_3460::kControl_WallData;
using build_3460::kCoopPinId;
using build_3460::kFieldTimeClient;
using build_3460::kHookArgSizeStackOffset;
using build_3460::kLayer_Chart;
using build_3460::kLayer_Playback;
using build_3460::kLoadKindMount;
using build_3460::kLoadingParkHeight;
using build_3460::kLoadingParkRadius;
using build_3460::kMaxStatEntries;
using build_3460::kMethod_Actor;
using build_3460::kMinimapIconCompanion;
using build_3460::kMinimapIconOutlinedArrow;
using build_3460::kMinimapIconPlayerArrow;
using build_3460::kMoveController_Method;
using build_3460::kMoveController_WallUp;
using build_3460::kMoveMethodWallUp;
using build_3460::kPhysics_AxisX;
using build_3460::kPhysics_AxisY;
using build_3460::kPhysics_AxisZ;
using build_3460::kPhysics_Position;
using build_3460::kPinColourCount;
using build_3460::kPinGroupNormal;
using build_3460::kPinShapeCount;
using build_3460::kPointClimbEnterHead;
using build_3460::kRelayedAcks;
using build_3460::kRemoteCatchSceneId;
using build_3460::kRideEnterHead;
using build_3460::kServerActor_Key;
using build_3460::kServerDockingWriterHead;
using build_3460::kServerOtherEquipWriterHead;
using build_3460::kStatContainer_Count;
using build_3460::kStatContainer_Entries;
using build_3460::kStatEntry_Current;
using build_3460::kStatEntry_Id;
using build_3460::kStatEntry_Max;
using build_3460::kStatEntry_Size;
using build_3460::kStatId_Health;
using build_3460::kStatId_Stamina;
using build_3460::kStatus_StatContainer;
using build_3460::kTroc_Kind;
using build_3460::kTroc_Opcode;
using build_3460::kVtCamera;
using build_3460::kVtPhysicsObject;
using build_3460::kWorldSystem_ActorManager;
using build_3460::kCamera_Block;
using build_3460::kLoadPositionFromEnd;
using build_3460::kLoadRotationFromEnd;
using build_3460::kOpSetFieldTimeReq;
using build_3460::kOpSetFieldTimeAck;
using build_3460::kOpActorLoadDataAck;
using build_3460::kOpRemoveActorAck;
using build_3460::kOpMoveActorReq;
using build_3460::kOpMoveActorAck;
using build_3460::kOpStartActionReq;
using build_3460::kOpStartActionAck;
using build_3460::kOpRotateActorReq;
using build_3460::kOpRotateActorAck;
using build_3460::kOpUpdateWallUpDataAck;
using build_3460::kOpUnequipItemAck;
using build_3460::kOpOtherEquipItemAck;
using build_3460::kOpOtherUnequipItemAck;
using build_3460::kOpCreateOrChangePinMarkerAck;
using build_3460::kOpRemovePinMarkerAck;
using build_3460::kOpChangeDockingSocketByFrameEventAck;
using build_3460::kOpRemoteCatchResultAck;
using build_3460::kOpUpdateOnDemandGimmickAck;
using build_3460::kStatus_HideFlags;
using build_3460::kHideFlagsOfCopies;
using build_3460::kStatusFlagsChangedHead;
using build_3460::kMovementMethodStepHead;
using build_3460::kPointClimbEnter;
using build_3460::kRideOnEnter;
using build_3460::kRideOffEnter;
using build_3460::kServerOtherEquipWriter;
using build_3460::kComponents_EquipSlot;
using build_3460::kEquipSlot_Serial;
using build_3460::kEquipSlot_Table;
using build_3460::kSlotTable_Entries;
using build_3460::kSlotTable_Count;
using build_3460::kSlotEntry_Size;
using build_3460::kSlotEntry_ItemIndex;
using build_3460::kSlotEntry_Count;
using build_3460::kSlotEntry_ShownIndex;
using build_3460::kSlotEntry_Slot;
using build_3460::kItemInfoTable_Count;
using build_3460::kItemInfoTable_Records;
using build_3460::kItemInfo_Key;
using build_3460::kServerDockingWriter;
using build_3460::kClientPostPacket;
using build_3460::kClientOnRecvPacket;

// ---- moved in 3479 ----------------------------------------------------------
inline constexpr uint32_t kPeTimestamp = 0x6AE6E3E8;  // 3460:0x6AE58024
inline constexpr uintptr_t kImageBase = 0x140000000;  // fixed, no ASLR
inline constexpr uintptr_t kImageSize = 0x14073000;  // 3460:0x13EE0000
inline constexpr uintptr_t kWorldSystemSlot = 0x71B6C50;  // 3460:0x71B5CA0 [world_system_ref, _p3: one match each]
inline constexpr uintptr_t kCameraSlot = 0x70D2F68;  // 3460:0x70D1FB0 [camera_global_ref]
inline constexpr uintptr_t kStaticPlayerPos = 0x71C5DF0;  // 3460:0x71C4E40 [player_static_pos_write]
inline constexpr uintptr_t kChunkOrigin = 0x71A2EB0;  // 3460:0x71A1F00 [world_offset_sub]
inline constexpr uintptr_t kClientClockGlobal = 0x70D2D75;  // 3460:0x70D1DB5; 15 cmp sites; live: flag 1, +0x3C 77672
inline constexpr uintptr_t kClientFrameClockMs = 0x70D2DA9;  // = flag + 0x34
inline constexpr uintptr_t kClientFrameClockUs = 0x70D2DB1;  // = flag + 0x3C
inline constexpr uintptr_t kClientClockBias = 0x71B6EF8;  // = kWorldSystemSlot + 0x2A8
inline constexpr uintptr_t kMinimapIconRoot = 0x71B6FE8;  // = kWorldSystemSlot + 0x398
inline constexpr uintptr_t kMinimapIconRootAlt = 0x71B6FF0;  // = kWorldSystemSlot + 0x3A0
inline constexpr uintptr_t kStatusFlagsChanged = 0x6E90A0;  // unchanged, head matches
inline constexpr uintptr_t kVtNormalMovementMethod = 0x5952BF8;  // 3460:0x5951BF8
inline constexpr uintptr_t kMovementMethodStep = 0xBBC570;  // 3460:0xBBC5A0 = slot 2 of NormalMovementMethod, head matches
inline constexpr uintptr_t kVtClientActorManager = 0x593ECF0;  // 3460:0x593DCF0
inline constexpr uintptr_t kVtControlledActor = 0x590CCA0;  // 3460:0x590BCA0
inline constexpr uintptr_t kVtCharacterControl = 0x593D8E0;  // 3460:0x593C8E0
inline constexpr uintptr_t kVtStatusComponent = 0x59146E0;  // 3460:0x59136E0
inline constexpr uintptr_t kVtWallUpMovementMethod = 0x591DF78;  // 3460:0x591CF78
inline constexpr uintptr_t kActionLayerAdvance = 0x214C070;  // 3460:0x214C0C0, 16-byte head: one match
inline constexpr uintptr_t kActionLayersAdvance = 0x232DF70;  // 3460:0x232DFE0, keeps its 0x670 gap to kPending (0x232E5E0)
inline constexpr uintptr_t kMinimapIconDecide = 0x9E959D;  // 3460:0x9E95CD, context signature: one match
inline constexpr uintptr_t kVtClientEquipSlotComponent = 0x5945388;  // 3460:0x5944388
inline constexpr uintptr_t kItemInfoTableSlot = 0x71B7BD0;  // 3460:0x71B6C20, decoded from the load below
inline constexpr uintptr_t kItemInfoTableLoad = 0x3DCFD5;  // unchanged
inline constexpr uintptr_t kTrocInstancesBegin = 0x6E87D10;  // 3460:0x6E86D10; live: 1175 handlers, same span 0x33660
inline constexpr uintptr_t kTrocInstancesEnd = 0x6EBB378;  // last + 8
inline constexpr uint8_t kMinimapIconDecideHead[] = {0x80, 0x7C, 0x24, 0x48, 0x00, 0x75, 0x6F, 0x8B, 0x05, 0x36, 0x84, 0x75, 0x06, 0x41, 0x89, 0x06};  // its mov eax,[rip+d] moved with the data

}  // namespace cdmp::game::build_3479
