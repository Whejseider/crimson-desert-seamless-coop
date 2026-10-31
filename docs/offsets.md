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
