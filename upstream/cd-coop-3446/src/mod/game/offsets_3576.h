#pragma once
// Addresses for CrimsonDesert.exe 1.0.0.3576 (Co-op+ port, 2026-11-06).
//
// Steam updated the game from 1.0.0.3549 (PE 0x6AEB7D59) to 1.0.0.3576 (PE 0x6AED2E85, 326,872,984 bytes) on
// 2026-11-06. Unlike 3527 -> 3549 the code moved this time. Every value was mapped with both exes on disk
// (build/port3576): code by its instructions with relative operands masked, data and vtables through the code
// that addresses them, vtables checked against RTTI, the 227 identical template copies by their order; the Troc
// opcode table is unchanged (1175 names, same ids).
//
// Shifts 3549 -> 3576 vary by region: code +0x250 (minimap decide) .. +0x2E0 (packet hooks, item load) ..
// +0x370 (character control) .. +0x660 (status, ride, climb) .. +0x820 (error logger, load steps) .. +0xC40
// (action layer, quests) .. +0xD50 (aggro, the action-layers template block); vtables +0x20A8 .. +0x2590; data
// +0x2000 (Troc table, field time) .. +0x2220.

#include "mod/game/offsets_3549.h"

namespace cdmp::game::build_3576 {

// Everything this patch did not move (opcodes, struct offsets, wire layout, heads that match).
using build_3549::RelayedAck;
using build_3549::kActionLayerAdvanceHead;
using build_3549::kActionLayersAdvanceHead;
using build_3549::kActorManager_ControlledActor;
using build_3549::kActor_ComponentTable;
using build_3549::kActor_Key;
using build_3549::kActor_OwnerUser;
using build_3549::kCameraBlockSentinelValue;
using build_3549::kCameraBlock_Forward;
using build_3549::kCameraBlock_Position;
using build_3549::kCameraBlock_Projection;
using build_3549::kCameraBlock_Sentinel;
using build_3549::kCameraBlock_View;
using build_3549::kCamera_Block;
using build_3549::kClientClock;
using build_3549::kClientClockHead;
using build_3549::kClientOnRecvPacketHead;
using build_3549::kClientPostPacketHead;
using build_3549::kComponents_CharacterControl;
using build_3549::kComponents_EquipSlot;
using build_3549::kComponents_Status;
using build_3549::kControlInner_Physics;
using build_3549::kControl_Inner;
using build_3549::kControl_Layer0;
using build_3549::kControl_MoveController;
using build_3549::kControl_WallData;
using build_3549::kCoopPinId;
using build_3549::kEquipSlot_Serial;
using build_3549::kEquipSlot_Table;
using build_3549::kErrorLoggerHead;
using build_3549::kHideFlagsOfCopies;
using build_3549::kHookArgSizeStackOffset;
using build_3549::kItemInfoTable_Count;
using build_3549::kItemInfoTable_Records;
using build_3549::kItemInfo_Key;
using build_3549::kLayer_Chart;
using build_3549::kLayer_Playback;
using build_3549::kLoadKindMount;
using build_3549::kLoadPositionFromEnd;
using build_3549::kLoadRotationFromEnd;
using build_3549::kLoadingParkHeight;
using build_3549::kLoadingParkRadius;
using build_3549::kMaxStatEntries;
using build_3549::kMethod_Actor;
using build_3549::kMiniGameHolder_Gauge;
using build_3549::kMiniGameRoot_Holder;
using build_3549::kMinimapIconCompanion;
using build_3549::kMinimapIconOutlinedArrow;
using build_3549::kMinimapIconPlayerArrow;
using build_3549::kMoveController_Method;
using build_3549::kMoveController_WallUp;
using build_3549::kMoveMethodWallUp;
using build_3549::kMovementMethodStepHead;
using build_3549::kOpActorLoadDataAck;
using build_3549::kOpChangeDockingSocketByFrameEventAck;
using build_3549::kOpCreateOrChangePinMarkerAck;
using build_3549::kOpMoveActorAck;
using build_3549::kOpMoveActorReq;
using build_3549::kOpOtherEquipItemAck;
using build_3549::kOpOtherUnequipItemAck;
using build_3549::kOpRemoteCatchResultAck;
using build_3549::kOpRemoveActorAck;
using build_3549::kOpRemovePinMarkerAck;
using build_3549::kOpRotateActorAck;
using build_3549::kOpRotateActorReq;
using build_3549::kOpSetFieldTimeAck;
using build_3549::kOpSetFieldTimeReq;
using build_3549::kOpStartActionAck;
using build_3549::kOpStartActionReq;
using build_3549::kOpUnequipItemAck;
using build_3549::kOpUpdateOnDemandGimmickAck;
using build_3549::kOpUpdateWallUpDataAck;
using build_3549::kPhysics_AxisX;
using build_3549::kPhysics_AxisY;
using build_3549::kPhysics_AxisZ;
using build_3549::kPhysics_Position;
using build_3549::kPinColourCount;
using build_3549::kPinGroupNormal;
using build_3549::kPinShapeCount;
using build_3549::kPointClimbEnterHead;
using build_3549::kRelayedAcks;
using build_3549::kRemoteCatchSceneId;
using build_3549::kRideEnterHead;
using build_3549::kServerActor_Key;
using build_3549::kServerDockingWriter;
using build_3549::kServerDockingWriterHead;
using build_3549::kServerOtherEquipWriter;
using build_3549::kServerOtherEquipWriterHead;
using build_3549::kSlotEntry_Count;
using build_3549::kSlotEntry_ItemIndex;
using build_3549::kSlotEntry_ShownIndex;
using build_3549::kSlotEntry_Size;
using build_3549::kSlotEntry_Slot;
using build_3549::kSlotTable_Count;
using build_3549::kSlotTable_Entries;
using build_3549::kStatContainer_Count;
using build_3549::kStatContainer_Entries;
using build_3549::kStatEntry_Current;
using build_3549::kStatEntry_Id;
using build_3549::kStatEntry_Max;
using build_3549::kStatEntry_Size;
using build_3549::kStatId_Health;
using build_3549::kStatId_Stamina;
using build_3549::kStatusFlagsChangedHead;
using build_3549::kStatus_HideFlags;
using build_3549::kStatus_StatContainer;
using build_3549::kTroc_Kind;
using build_3549::kTroc_Opcode;
using build_3549::kVtCamera;
using build_3549::kVtPhysicsObject;
using build_3549::kWorldSystem_ActorManager;

// ---- moved in 3576 ----------------------------------------------------------
inline constexpr uint32_t kPeTimestamp = 0x6AED2E85;  // 3549:0x6AEB7D59
inline constexpr uintptr_t kImageBase = 0x140000000;  // fixed, no ASLR
inline constexpr uintptr_t kImageSize = 0x13CC4000;  // 3549:0x13CFF000
inline constexpr uintptr_t kWorldSystemSlot = 0x71C0100;  // 3549:0x71BDEF0, 8/8 code references agree
inline constexpr uintptr_t kCameraSlot = 0x70DC130;  // 3549:0x70DA028, 2/2
inline constexpr uintptr_t kStaticPlayerPos = 0x71CF140;  // 3549:0x71CD0B0, 2/2
inline constexpr uintptr_t kChunkOrigin = 0x71AC2C0;  // 3549:0x71AA150, 8/8
inline constexpr uintptr_t kCutscenePlaying = 0x70DBF96;  // 3549:0x70D9F14, 2/2
inline constexpr uintptr_t kFieldTimeClient = 0x6EAD998;  // 3549:0x6EAB998, 6/6
inline constexpr uintptr_t kMiniGameRoot = 0x71C0158;  // 3549:0x71BDF48, 8/8
inline constexpr uintptr_t kClientClockGlobal = 0x70DBF2D;  // 3549:0x70D9E3D, 8/8 cmp sites
inline constexpr uintptr_t kClientFrameClockMs = 0x70DBF61;  // = flag + 0x34
inline constexpr uintptr_t kClientFrameClockUs = 0x70DBF69;  // = flag + 0x3C
inline constexpr uintptr_t kClientClockBias = 0x71C03A8;  // = kWorldSystemSlot + 0x2A8 (8/8 references agree)
// Co-op+ low latency (2026-11-06): the client's game clock in ms, one u64. Found live with the dev clockscan
// (dev_memscan.h): it runs with the wall clock and stays ~54 ms ahead of the newest MoveActorReq stamp (the game stamps
// its Reqs that much behind it), unlike flag+0x34 above (0.7 s off). game_clock.cpp reads it on this build.
inline constexpr uintptr_t kClientClockMs = 0x7233A78;
inline constexpr uintptr_t kMinimapIconRoot = 0x71C0498;  // = kWorldSystemSlot + 0x398
inline constexpr uintptr_t kMinimapIconRootAlt = 0x71C04A0;  // = kWorldSystemSlot + 0x3A0
inline constexpr uintptr_t kStatusFlagsChanged = 0x6EA030;  // 3549:0x6E99D0, head matches
inline constexpr uintptr_t kVtNormalMovementMethod = 0x595AD08;  // 3549:0x5958C10 (= RTTI); slot 2 = kMovementMethodStep
inline constexpr uintptr_t kMovementMethodStep = 0xBBDA10;  // 3549:0xBBD5F0, head matches
inline constexpr uintptr_t kVtClientActorManager = 0x5946C40;  // 3549:0x5944B38 (= RTTI)
inline constexpr uintptr_t kVtControlledActor = 0x5914D58;  // 3549:0x59127C8 (= RTTI ClientChildOnlyInGameActor)
inline constexpr uintptr_t kVtCharacterControl = 0x59459F0;  // 3549:0x59438F0 (= RTTI)
inline constexpr uintptr_t kVtStatusComponent = 0x591C7C0;  // 3549:0x591A710 (= RTTI)
inline constexpr uintptr_t kVtWallUpMovementMethod = 0x5926050;  // 3549:0x5923FA8 (= RTTI)
inline constexpr uintptr_t kActionLayerAdvance = 0x214F8D0;  // 3549:0x214EC90, head matches
inline constexpr uintptr_t kActionLayersAdvance = 0x2332070;  // 3549:0x2331320; one of 227 identical template copies: the 64th in both builds (the whole block moved +0xD50, its callees too), 0x670 gap to kPending kept
inline constexpr uintptr_t kMinimapIconDecide = 0x9EA56D;  // 3549:0x9EA31D
inline constexpr uintptr_t kVtClientEquipSlotComponent = 0x594D478;  // 3549:0x594B388 (= RTTI)
inline constexpr uintptr_t kItemInfoTableSlot = 0x71C1080;  // 3549:0x71BEE70, decoded from the load below
inline constexpr uintptr_t kItemInfoTableLoad = 0x3DD8B5;  // 3549:0x3DD5D5
inline constexpr uintptr_t kTrocInstancesBegin = 0x6E90D10;  // 3549:0x6E8ED10, same span
inline constexpr uintptr_t kTrocInstancesEnd = 0x6EC4378;  // last + 8
inline constexpr uintptr_t kGameplayInput = 0x71A16C8;  // 3549:0x719F558, u8
inline constexpr uintptr_t kUiModePush = 0xF22247;  // 3549:0xF21B07; its movzx reads kGameplayInput
inline constexpr uintptr_t kErrorLogger = 0x15FCC70;  // 3549:0x15FC450, head matches
inline constexpr uintptr_t kClientPostPacket = 0x3A6310;  // 3549:0x3A6030, head matches
inline constexpr uintptr_t kClientOnRecvPacket = 0x3A66A0;  // 3549:0x3A63C0, head matches
inline constexpr uintptr_t kPointClimbEnter = 0x761070;  // 3549:0x760A10, head matches
inline constexpr uintptr_t kRideOnEnter = 0x750210;  // 3549:0x74FBB0, head matches
inline constexpr uintptr_t kRideOffEnter = 0x7511E0;  // 3549:0x750B80, head matches
inline constexpr uint8_t kMinimapIconDecideHead[] = {0x80, 0x7C, 0x24, 0x48, 0x00, 0x75, 0x6F, 0x8B, 0x05, 0x26, 0x08, 0x76, 0x06, 0x41, 0x89, 0x06};  // its mov eax,[rip+d] moved with the data

}  // namespace cdmp::game::build_3576
