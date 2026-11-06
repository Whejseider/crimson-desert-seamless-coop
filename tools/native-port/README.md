# Native Server port tools (build 2976 -> 1.0.0.3549)

No 2976 exe exists here, so addresses are found in the archived 3549 exe (`build/exe-archive/`) by:
- `heads.py`: every `constexpr uintptr_t kX` in upstream/cdmp-native with a recorded byte array, searched exactly
  (unique / several / none) -> heads.json.
- `vt49.py`: vtables from RTTI type names (`.?AVName@pa@@`), and their slots.
- `xref49.py`: RIP-relative and call/jmp references to an address (numpy, ~2 s per target) and the function around
  an address (.pdata).
- `dis49.py`: capstone disassembly (`python dis49.py <hex rva> [count]`).
- `resolve.py`: the old base's per-build offset tables resolved by name, paired with the native 2976 values.
Results with evidence: docs/research/native-port-3549.json. Plan: docs/research/native-port-plan.md.
Methods that worked: functions close together keep their order and spacing (ToNear +0x20 / +0x150); globals in one
.data region share a shift (+0x454D60 around WorldSystem); a sequence with wildcards over its displacements decodes
the global it reads; a vtable's slot confirms a function picked by bytes. ByJunter's Ghidra name list for 2976:
upstream/cdmp-native/docs/research/ghidra/scripts/data/known_names_repo.csv.
