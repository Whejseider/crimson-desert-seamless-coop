# Offsets and signatures

Rules: signatures (AOB) over fixed addresses; every entry records the game build it was verified on, how it was found,
and its RVA on that build. Probe them via `config/probe.toml` before using them in code.

Game builds seen (from cdmp / cd-coop-3446): 1.0.0.2976 (patch 2.03.02, PE timestamp 0x6AB28F00, image base
0x140000000), 1.0.0.3424, 1.0.0.3446. Patch 2.04.00 (DLC) around 2026-10-30. Exe: `bin64\CrimsonDesert.exe`.

| Name | Pattern | Resolve | Verified build | RVA | Source | Notes |
|---|---|---|---|---|---|---|
| (none verified by us yet) | | | | | | M0 in Phase B |

Leads (not verified by us): cdmp `src/common/*_2976.*` tables and `docs/research/cheat-tables-versions.md`,
`orcax-equiphide.md` (WorldSystem pointer chain to the local player, validated by RTTI), `troc-dispatch.md`
(`ClientNet::PostPacket`, `ClientNet::OnRecvPacket`). GPL-3: if we copy their signatures into our files, that part is
derived from GPL code (see DECISIONS).

## Audit of the 3446 addresses against the installed exe (2026-10-31, offline, read-only)
Method: `scripts/verify-port.ps1` plus `cdmp_probe --image` (RTTI `vtables`, `dump`, `disasm`) on
`D:\SteamLibrary\steamapps\common\Crimson Desert\bin64\CrimsonDesert.exe` (1.0.0.3446, PE 0x6AE46DD0).

| Address | Status | Effect |
|---|---|---|
| All of verify-port's list (globals, Troc range, send/recv hooks, minimap, equip writer, containment, RTTI) | PASS | - |
| kVtClientActorManager 0x5939CD8, kVtCharacterControl 0x59388D0 | match their RTTI vtables | - |
| **kPointClimbEnter, kRideOnEnter, kRideOffEnter** | **were 0x10 too high** (mid-function): fixed to 0x75F620 / 0x74E8D0 / 0x74F8A0, now checked by verify-port | the puppet's wall-climb fix and mount fix would never have installed ("wall climbs will freeze") |
| kClientClock 0x1416B80 | known stale (upstream) | time-of-day sync off |
| game_errors kErrorLogger 0x35D8D0 | stale (byte 0x00, expected jmp) | the game's own error lines are not copied into cdmp.log; nothing else |
| aggro_test kIsTargetable / kSetCurrentTarget / kCheckAIBattleType | stale 2976 values, head checks fail | only `[dev] aggro_test` and `[coop] enemies_target_copy` (both off by default) |
| remote_puppet rope lookup / bend (0x6D69190, 0x1851010, 0x86A576, 0x59D89A ...) | 2976 values | only `[dev] rope_piece_lookup` / `rope_bend` (off by default); the bend hook has a head check |
| kServerDockingWriter (inherited from 3424) | stale, but the docking relay is compiled out (kDockingRelay = false) | none |
| Camera block offsets kCamera_Block 0x428, sentinel 0xAC0 (inherited from 3424) | **unverified on 3446**; only checkable live | if wrong: panel shows "Camera: camera block sentinel mismatch" and nameplates cannot be drawn (M0 step 3) |
| Everything else inherited from 3424 (struct field offsets, opcodes, stat ids) | not address-like; opcodes spot-checked equal | - |
