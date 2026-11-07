# Reverse-engineering reference, game 1.0.0.3576 (2026-11-07)

One place for what was learnt about the game's code and messages on 3576 while building shared XP, the enemy attacks
on the guest, the stagger and the crash guards. Dated entries with the full story are in PROGRESS.md (2026-11-07);
this page is the lookup table. All addresses are RVAs (image base 0x140000000). After a game update every address here
must be found again: the method section says how.

## Method

- **Ghidra 12.2** (needs JDK 25; Temurin 25 installed by the user) unpacked in `build/tools/` (git-ignored), settings
  and cache redirected to `build/tools/ghidra-settings|ghidra-cache` (`support/launch.properties` VMARGS; the default
  AppData OSGi cache failed with "The data file must be inside the data dir").
- No full auto-analysis (the exe is 327 MB): `build/tools/scripts/Decomp.java` decompiles chosen RVAs only:
  `analyzeHeadless build/tools/ghidra-proj cd3576 -process CrimsonDesert-1.0.0.3576.exe -noanalysis -scriptPath
  build/tools/scripts -postScript Decomp.java <out.c> <rva...>` with JAVA_HOME = the JDK 25 (first run: `-import
  build/exe-archive/CrimsonDesert-1.0.0.3576.exe` instead of `-process`).
- Python helpers (`build/port3479/pe.py`, `vt.py`; `CDMP_EXE` selects the exe):
  - a request's handler: TrocTr class vtable slot 2 (`vt.vtable('.?AVTrocTr<Name>@pa@@')`, `slot(v, 2)`);
  - who writes a message: search executable sections for the opcode as an immediate (`mov word [r], 0xBB3`);
  - a function's start: back from an address to `CC CC` padding (fails inside the protected `.tls$` / `.xtls` code,
    which has no padding and is deliberately scrambled);
  - callers: `E8 rel32` to the start; class of a function: find the 8-byte pointer in a vtable, RTTI COL before it.
- Live checks: two copies of the game on one PC (`build/port3527/twocopy_cp.sh`), sandbox saves only, packet
  recordings (`[sniffer] record=1`, read with `tools/port/pkbin.py`), SAVES OK checked after every run.

## Opcodes used (same ids on 3549 and 3576)

| Message | Id | Notes |
|---|---|---|
| AddSubLevelExperienceAck | 2995 (0xBB3) | `u32 player \| i64 max \| u8 0 \| u32 sub-level \| u64 amount \| u16` (27 B) |
| ProcessSubLevelIncreaseExperienceOnceTimer | 2069 | server-internal timer; see "Experience" |
| AddExperienceAck / AddExperienceByGmReq | 3152 / 2467 | character experience, unused by the game |
| VarySubLevelExpByResourceReq | 2918 | spends a resource |
| DoFrameEventReq | 3030 | the client's hit report; 1335 B for a hit (first byte 04) |
| AttackByCollisionReq | 2150 | `u32 victim \| u32 attacker \| u32 0 \| vec3 point \| vec3 dir \| u32 projectile \| u32 0`; only collision keys (0xF2EE2678) are accepted; the arrow keys (0xF653A168, 0x0ED8D272) get no answer |
| CreateProjectileFromFrameEventReq | 2644 | a real arrow, from the bow's action frame (236 B) |
| CreateProjectileAck | 2737 | real arrow: `+4 0x0ED8D272 \| 0x000F4F01 \| data 0x0BB2BA9B \| ...target`; the collision "projectile" is all zeros (no damage) |
| OnHitProjectileReq | 2860 | `u32 player \| u32 arrow n \| u32 owner \| u32 target \| vec3 point` |
| PostProcessAttackResult_AttackTargetAck (ATA) | 2791 | 219 B: `+0 target \| +4 attacker \| +8 flags \| +0x24 attack hash` |
| ApplyActionRestrictionAck (AAR, the stagger) | 3059 | **3576: 69 B, instance +0x1C, source +0x41** (2976: 77 B, +0x24, +0x49) |
| ApplyBuffAck | 2677 | 77 B: `+0 holder \| +4 buff \| +0x14 instance \| +0x18 source \| +0x1C target \| +0x29 pos \| +0x39/+0x41 server ms`; stagger buff 0x452, damage buffs 1021/1023 |
| VaryStatCheatReq | 2326 | `u32 key \| u32 stat (HP 0x000F4240) \| u8 mode \| i32 value \| u8 \| u8`; mode 0 = add (exact); mode 1 is NOT "set to" (63.9 -> 63.0 HP for value 1); HP 0 leaves the enemy "fatal" (no kill credit) |
| VaryHPAck | 2740 | `+0 key \| +4 stat \| +8 hp x1000 \| ... \| +0x18 previous hp x1000` |
| NotifyHandleDeadAck | 2985 | `u32 victim \| u32 killer \| ...` |
| MissionTriggerInOutReq | 2899 | the harmless mission-0 trigger that wakes the quest server's thread |

## Experience (shared XP, `net/xp_share.*`, `quest_advance.cpp`)

- Kill experience = sub-level experience: KillActor fills a skill-point bar (521 Kliff, 522 Oongka, 523 Damiane):
  Tier_1 10, Tier_2 15, Tier_3 25, Elite 140, Boss 1000; only after Mission_Intro_Abyss_Tutorial (upstream research).
- XP message writer **0x2CF56D0** (`mov word [r15], 0xBB3` at 0x2CF5853); called by the experience routine
  **0x2EB4180**(sub-level manager, u32* out, actor, u16* index, u64 amount, u8 flag) (from 0x2EB4795) and 0x2EB5600.
- 0x2EB4180's callers: **0x2ED9000** = handler of ProcessSubLevelIncreaseExperienceOnceTimer (TrocTr vtable 0x5EFDF30,
  slot 2) and protected code at .tls$ 0xE64D8EF (probably the KillActor path; not decompilable cleanly).
- 0x2ED9000(handler, u32* out, int64* ctx): ctx[0] = the user = **the player's server actor** (key at +0x60,
  components at +0x68: +0x28 sub-level manager, +0x1A0 lock object), ctx[3] = the message:
  `u32 actor at +3 (0 = the user) | u32 sub-level KEY at +7 | u64 amount at +0x0B | u8 flag at +0x13` (the key is
  turned into the index through the table at [exe+0x71C3B50]). Returns 0 in *out on success.
- Server actor lookup (as MissionTriggerInOutReq's handler 0x2EEF470 does): **0x2D11AA0**(manager =
  [[exe+0x71C0610]+0x50], handle*, u32 key): handle +8 = actor, +0x10 = found; release: write exe+0x5AC58E0 at handle+0,
  call **0x1618660**(handle).
- The mod calls 0x2ED9000 on the quest server's thread (mid-hook at 0x2EEF470's entry), under SEH. gx1 (wrong user)
  faulted harmlessly; gx2 / xs1: the server answers with the same AddSubLevelExperienceAck as a kill.

## Hits, projectiles, stagger (`guest_hits.cpp`)

- DoFrameEventReq's reader 0x2BE1C10 -> **0x2B5A200** dispatches on the hit type byte: 0 -> 0x2D19830 (checks the
  attacker's CURRENT action hash and frame: a fake melee hit only works while the player really swings), 1 ->
  0x2E857C0 (a projectile's hit: checks only that the projectile exists in the attacker's table and its data entry;
  no visibility or distance check seen), 3 -> 0x2D19E90.
- The client flies projectiles and reports their hits itself (OnHitProjectileReq + DoFrameEventReq): a hit on an actor
  the client does not show (a hidden native) hits nothing.
- The collision blow (AttackByCollisionReq 0xF2EE2678) makes an all-zero "projectile": a buff (OnPostBuffDamagedAck,
  ApplyBuffAck 0x3FF), **no damage** on 3576 -> the host-side "finishing collision" never kills; the mod's KillCheatReq
  fallback kills without XP (hence `xp_share_partner_finished`).
- Built-in reaction templates (a real sword hit on a bandit, recording rec/cdmp_packets_20261106-235658): kDefaultAta
  (219 B), kDefaultAar (69 B), kDefaultBuff (77 B, buff 0x452).

## Enemy AI and the guest's copy (`aggro_test.cpp`)

- CheckAIBattleType: vtable slot 1 = 5-byte `jmp` at **0x24C2B50** into .xtls **0xD06B630**(cond, obj_ia, ctx):
  target at **ctx+0x268**, valid byte **ctx+0x270** (2976: +0x260/+0x268); the asking enemy through a handle:
  **0x192EF10**([[[obj_ia+0x68]]+8], out) -> out+8 = the actor (release as above). Target's CommonTarget component
  = [comps+0x90]: rank lists +0x58+16t (counts +0x60/+0x70/+0x80/+0x90); battle byte **+0x164**.
- The copy is only "in battle" if +0x164 = 1 (the game sets it only for its own characters): kept at 1 on the host
  (`aggro_keep_copy_in_battle`); then the AI asks CheckAIBattleType about the copy and the override lets 2 attack.

## Crash guards (`horse_diag.cpp`)

- Character component (CC): actor at +0x08; layers at +0x88 + 8i; layer: current action +0x10, playing +0x18, chart
  +0x110. A copy can load with layers that have a chart and no action.
- **0xA128C0**([CC*]): processes queued animation events; reads [layer+0x10] -> +0x111/+0x115 with no null check on
  one path (crash 0xA12E4E reading 0x115). Skipped for copies while any layer 0-3 has a chart and no action.
- 0x2518850 (action playback, crash 0x2519762 reading 0x28, seen once): not guarded yet.
- CC tick 0x9451E0, "still loading?" 0x9439B0, animation update 0x976B70 (existing guards).

## Other 3576 facts found today

- Starter horse: user-owned load kind 4 (slot 1); slot-2 mounts kind 5. Call: hold D-pad Up + right stick down
  (H does nothing); the horse teleports near (it moved 323 m in < 5 s).
- A new game's first saves: 0 then 1 completed missions.
- The sync digest must not count dead copies kept for loot (false resyncs gave back all hidden enemies).

## Items (2026-11-07, the decomp pass; unblocks T18 trading / T45 stash / T26 penalties)

- Every request handler in the TrocTr table starts the same way (generated code): check the size, then
  `if (*(char*)(handler + 0x21) == 0) { read fields; do it }` else **silently answer 0 (success) and do nothing**.
  In the retail exe the cheat / dev requests have +0x21 set: that is why AddMinusItemReq got "no answer at all" in
  the T18 test (addminus1), not a wrong layout.
- AddMinusItemReq (handler **0x2C9A090**, vtable 0x5EEC9F8): `u32 item key | i64 count`, then
  **0x2E08220**(inventory, u32* err, u32 item key, i64 count) with inventory = `[[user + 0x68] + 0xB8]` (user = the
  player's server actor, as for XP). 0x2E08220: key 0 -> error; key -> u16 index through 0x3D7E10 (-1 = unknown item
  -> error); owner = [inventory + 8]; locks [owner + 8]; **0x22FE820**(inventory, key, count, INT64_MAX) changes the
  count; then 0x2C89CA0(owner, msg, 2 if count > 0 else 1, ...) sends the client its inventory update; *err = 0.
  On an error the handler sends message 0x3F5 (1013, an error notice) with the code.
  CORRECTION (same day): the message 0x2C89CA0 sends is 0xBB6 = 2998 = **MinusItemDataListAck**, and 0x22FE820 only
  keeps a `{u32 key, i64 count}` tally at inventory+0xF8 (removed at <= 0). So AddMinusItemReq edits a "minus item"
  list, NOT the bag; it does not give or take a real item. (Not tested live; nothing built on it.)
- The real bag notifier: **0x2C75730** writes both AddItemToInventoryAck (3110 = 0xC26, at 0x2C758FB) and
  RemoveItemToInventoryAck (2421 = 0x975, at 0x2C75FA3). Its 5 callers: 0x29EBBC0, 0x2B5D900, 0x2DF04A0,
  **0x2E05550**(obj, int* err, ?, u32, u16, u32, u32, u64 -- the shape of an add-to-inventory worker), 0x2E0DC70.
  Tool: build/port3479/opwriters.py (who writes an opcode), callers2.py (E8 callers).
- 0x2E05550 read in full (build/decomp/items5.c): NOT a plain add. It is an **exchange / conversion** worker:
  `(obj, int* err, ?, u32, u16 recipe, u32 from_key, u32 to_key, u32 times)`. It looks the u16 up in the table at
  [0x71CF2F0] (hash buckets of 0x100 bytes, entries `{.., u16 id @+4, u16 index @+6}`), maps both item keys to
  indices with 0x3D7E10 (the item hash map), checks the two rates (+0x28 of the recipe record from 0x0AD2BD0, max 0x28
  per step), then for each step: builds the new items as 200-byte item records (0x25FED70(record, &index, count),
  pushed by 0x2600090), removes the cost with 0x29ECDE0(bag+0x80, ...) and **adds with
  0x29EA7C0(bag, &err, ?, &records, 1, expire_time, flags, ..., 5, 0, 1, 1, 0)**, commits with 0x29EAD70 /
  0x29ECF00 / 0x29F10C0 / 0x29ED3D0 / 0x29EB3A0 / 0x29ED4F0 / 0x29EBA00 / 0x29EDAA0 / 0x29EC0F0, then tells the
  client: 0x2C75FE0 (removed), 0x2C75730 (added), 0x2C89CA0 (minus-item list). So the bag's core add is
  **0x29EA7C0** (bag = the object at +0x2E8 of the worker's own frame, `lStack_2e8`, from 0x2E5D430 on the server
  thread's [tls+0x250]). Giving an item would mean building records with 0x25FED70 and the whole commit chain: too
  many unnamed steps to do safely without the full index; NOT built.
- Other handlers read on the way (same +0x21 gate): SpawnItemToGroundByCheatReq 0x2C8B360 (`u32 | u64 | u16 | item
  value struct` -> 0x2EE98D0 -> 0x2DD9600), PushEntitlementItemReq 0x2C98040 (`u16 | u32` -> 0x2B3D920 on
  [comps + 0x168]), TransferItemReq 0x2C96A10 (`u16 | u16 | u16 | u16` -> 0x2EEB9A0 -> 0x2D6D410 on [comps + 0x38]),
  CreateItemFromTrItemValueCheatReq 0x2C8B130 (a 0x1C0-byte item value read by 0x2642100 -> 0x2EE96F0),
  CreateItemAndEquipXXXOnceTimer 0x2ED9FF0 (-> 0x2DFBF40).
