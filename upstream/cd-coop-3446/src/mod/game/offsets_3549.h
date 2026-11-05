#pragma once
// Addresses for CrimsonDesert.exe 1.0.0.3549 (Co-op+ port, 2026-11-04).
//
// Steam updated the game from 1.0.0.3527 (PE 0x6AEA07A6) to 1.0.0.3549 (PE 0x6AEB7D59, 327,116,696 bytes) on
// 2026-11-04. The game's own code did not move: of the main code section 0.075% of the bytes differ, almost all
// of them 2-3 byte call targets into the copy protection (which is re-encrypted every build). Every address the
// mod uses (64: code by its instructions, data and vtables through the code that addresses them) maps to itself
// with identical bytes (build/port3549/map_all.py, map_3549.json). So this table is 3527's with the new PE
// timestamp and image size; live checks: docs/research/port-3549.md.

#include "mod/game/offsets_3527.h"

namespace cdmp::game::build_3549 {

// Unchanged from 3527 (every one).
using build_3527::RelayedAck;
using build_3527::kActionLayerAdvance;
using build_3527::kActionLayerAdvanceHead;
using build_3527::kActionLayersAdvance;
using build_3527::kActionLayersAdvanceHead;
using build_3527::kActorManager_ControlledActor;
using build_3527::kActor_ComponentTable;
using build_3527::kActor_Key;
using build_3527::kActor_OwnerUser;
using build_3527::kCameraBlockSentinelValue;
using build_3527::kCameraBlock_Forward;
using build_3527::kCameraBlock_Position;
using build_3527::kCameraBlock_Projection;
using build_3527::kCameraBlock_Sentinel;
using build_3527::kCameraBlock_View;
using build_3527::kCameraSlot;
using build_3527::kCamera_Block;
using build_3527::kChunkOrigin;
using build_3527::kClientClock;
using build_3527::kClientClockBias;
using build_3527::kClientClockGlobal;
using build_3527::kClientClockHead;
using build_3527::kClientFrameClockMs;
using build_3527::kClientFrameClockUs;
using build_3527::kClientOnRecvPacket;
using build_3527::kClientOnRecvPacketHead;
using build_3527::kClientPostPacket;
using build_3527::kClientPostPacketHead;
using build_3527::kComponents_CharacterControl;
using build_3527::kComponents_EquipSlot;
using build_3527::kComponents_Status;
using build_3527::kControlInner_Physics;
using build_3527::kControl_Inner;
using build_3527::kControl_Layer0;
using build_3527::kControl_MoveController;
using build_3527::kControl_WallData;
using build_3527::kCoopPinId;
using build_3527::kCutscenePlaying;
using build_3527::kEquipSlot_Serial;
using build_3527::kEquipSlot_Table;
using build_3527::kErrorLogger;
using build_3527::kErrorLoggerHead;
using build_3527::kFieldTimeClient;
using build_3527::kGameplayInput;
using build_3527::kHideFlagsOfCopies;
using build_3527::kHookArgSizeStackOffset;
using build_3527::kImageBase;
using build_3527::kItemInfoTableLoad;
using build_3527::kItemInfoTableSlot;
using build_3527::kItemInfoTable_Count;
using build_3527::kItemInfoTable_Records;
using build_3527::kItemInfo_Key;
using build_3527::kLayer_Chart;
using build_3527::kLayer_Playback;
using build_3527::kLoadKindMount;
using build_3527::kLoadPositionFromEnd;
using build_3527::kLoadRotationFromEnd;
using build_3527::kLoadingParkHeight;
using build_3527::kLoadingParkRadius;
using build_3527::kMaxStatEntries;
using build_3527::kMethod_Actor;
using build_3527::kMiniGameHolder_Gauge;
using build_3527::kMiniGameRoot;
using build_3527::kMiniGameRoot_Holder;
using build_3527::kMinimapIconCompanion;
using build_3527::kMinimapIconDecide;
using build_3527::kMinimapIconDecideHead;
using build_3527::kMinimapIconOutlinedArrow;
using build_3527::kMinimapIconPlayerArrow;
using build_3527::kMinimapIconRoot;
using build_3527::kMinimapIconRootAlt;
using build_3527::kMoveController_Method;
using build_3527::kMoveController_WallUp;
using build_3527::kMoveMethodWallUp;
using build_3527::kMovementMethodStep;
using build_3527::kMovementMethodStepHead;
using build_3527::kOpActorLoadDataAck;
using build_3527::kOpChangeDockingSocketByFrameEventAck;
using build_3527::kOpCreateOrChangePinMarkerAck;
using build_3527::kOpMoveActorAck;
using build_3527::kOpMoveActorReq;
using build_3527::kOpOtherEquipItemAck;
using build_3527::kOpOtherUnequipItemAck;
using build_3527::kOpRemoteCatchResultAck;
using build_3527::kOpRemoveActorAck;
using build_3527::kOpRemovePinMarkerAck;
using build_3527::kOpRotateActorAck;
using build_3527::kOpRotateActorReq;
using build_3527::kOpSetFieldTimeAck;
using build_3527::kOpSetFieldTimeReq;
using build_3527::kOpStartActionAck;
using build_3527::kOpStartActionReq;
using build_3527::kOpUnequipItemAck;
using build_3527::kOpUpdateOnDemandGimmickAck;
using build_3527::kOpUpdateWallUpDataAck;
using build_3527::kPhysics_AxisX;
using build_3527::kPhysics_AxisY;
using build_3527::kPhysics_AxisZ;
using build_3527::kPhysics_Position;
using build_3527::kPinColourCount;
using build_3527::kPinGroupNormal;
using build_3527::kPinShapeCount;
using build_3527::kPointClimbEnter;
using build_3527::kPointClimbEnterHead;
using build_3527::kRelayedAcks;
using build_3527::kRemoteCatchSceneId;
using build_3527::kRideEnterHead;
using build_3527::kRideOffEnter;
using build_3527::kRideOnEnter;
using build_3527::kServerActor_Key;
using build_3527::kServerDockingWriter;
using build_3527::kServerDockingWriterHead;
using build_3527::kServerOtherEquipWriter;
using build_3527::kServerOtherEquipWriterHead;
using build_3527::kSlotEntry_Count;
using build_3527::kSlotEntry_ItemIndex;
using build_3527::kSlotEntry_ShownIndex;
using build_3527::kSlotEntry_Size;
using build_3527::kSlotEntry_Slot;
using build_3527::kSlotTable_Count;
using build_3527::kSlotTable_Entries;
using build_3527::kStatContainer_Count;
using build_3527::kStatContainer_Entries;
using build_3527::kStatEntry_Current;
using build_3527::kStatEntry_Id;
using build_3527::kStatEntry_Max;
using build_3527::kStatEntry_Size;
using build_3527::kStatId_Health;
using build_3527::kStatId_Stamina;
using build_3527::kStaticPlayerPos;
using build_3527::kStatusFlagsChanged;
using build_3527::kStatusFlagsChangedHead;
using build_3527::kStatus_HideFlags;
using build_3527::kStatus_StatContainer;
using build_3527::kTrocInstancesBegin;
using build_3527::kTrocInstancesEnd;
using build_3527::kTroc_Kind;
using build_3527::kTroc_Opcode;
using build_3527::kUiModePush;
using build_3527::kVtCamera;
using build_3527::kVtCharacterControl;
using build_3527::kVtClientActorManager;
using build_3527::kVtClientEquipSlotComponent;
using build_3527::kVtControlledActor;
using build_3527::kVtNormalMovementMethod;
using build_3527::kVtPhysicsObject;
using build_3527::kVtStatusComponent;
using build_3527::kVtWallUpMovementMethod;
using build_3527::kWorldSystemSlot;
using build_3527::kWorldSystem_ActorManager;

// ---- changed in 3549 ----------------------------------------------------------
inline constexpr uint32_t kPeTimestamp = 0x6AEB7D59;  // 3527:0x6AEA07A6
inline constexpr uintptr_t kImageSize = 0x13CFF000;  // 3527:0x13EB2000

}  // namespace cdmp::game::build_3549
