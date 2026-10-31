# Issue for nouzert1/cd-coop-3446 -- POSTED 2026-10-31 with the user's OK: https://github.com/nouzert1/cd-coop-3446/issues/1

**Title:** 3460: MoveData layout changed, opcode constants are 2976-era, camera block +0x430 (findings to share)

Hi! I'm working on a fork built on your 3446 port (credited, GPL). Our game updated to 1.0.0.3460 today too, and
since you've started porting, here is what we measured on the running 3460 game, in case it saves you time.
Your part-1 addresses match ours everywhere we overlap.

1. **MoveActor/StartAction MoveData was reordered and grew 4 bytes** (MoveActorReq 514 -> 518, StartActionReq
   725 -> 729; RotateActorReq unchanged). From the client parser (MoveActorAck vf2 exe+0xC6F2A0 -> MoveData reader
   exe+0x26356B0 -> Path exe+0x148A940): the Path now comes first. Req offsets: Path.local pos 0x04 / rot 0x10 /
   scale 0x20, parent u64 0x44, Path.world 0x4C (block 0x40), rot 0xB4, pos 0xC4, u32 0xD0, ts u64 0xD4, new u32 0xDC,
   MoveData end 0xE2 (Ack = req[0:0xE2] + u8 + req[0xE2:0xEE], 239 B). StartAction: MoveData still at 0x8E, flag
   0x2D8, fields = Move offset - 4 + 0x8E. Confirmed on captures.
2. **All kOp* constants in the tree are 2976-era numbers.** The sniffer re-resolves eight by name, but
   local_capture/enemy_relay/guest_hits/loot_twin/native_suppress/player_hits compare against the constants, so on
   3446 they matched unrelated messages (e.g. local_capture never saw MoveActorReq). Full 3460 table (name, opcode,
   kind for all 1175 handlers) attached.
3. **Camera block pointer moved to camera+0x430** (was 0x428); the block itself is unchanged.
4. **ActorLoadDataAck transform is at end-158 / rotation end-146** (was 154/142).
5. **Open problem we hit next:** with all of the above, the player copy spawns in place (name plate, map pin) and
   movement Acks are injected byte-correct, but actor 0xA010F001 stops at u16 [actor+0x5E] = 0x08 /
   [actor+0xCF] = 0 while its user (0x9010F001, +0x5E = 4) and body (0xB010F001, 0x10 / 1) are fine, so the client
   keeps its Acks queued. The 8 -> 0x10 setter is vtable slot +0x78 of ClientChildOnlyInGameActor (exe+0x1618B60).
   If you know what drives that step, we'd love to hear it.

Happy to open a PR with any of this if useful.
