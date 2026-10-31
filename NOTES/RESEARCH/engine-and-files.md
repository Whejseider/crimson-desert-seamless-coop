# Crimson Desert — Engine, Executable, Versions, File Formats

Retrieved: 2026-10-31 (all sources below retrieved on this date unless noted).
Method: WebSearch + WebFetch. Most news sites (kitguru, pcgamesn, mp1st, dsogaming, pcgamingwiki,
thegameswiki, digitaltoday, crimsondesert.pearlabyss.com, patchbot.io) were NOT fetchable from this
environment (DNS/allowlist), nexusmods/reddit blocked, web.archive.org refused. For those, only
search-result snippets were used and are marked "(snippet)". GitHub pages/raw files were fetchable.

## Key takeaways

- **Engine:** Pearl Abyss in-house **BlackSpace Engine** (proprietary; no public SDK/docs). PC renderer is **Direct3D 12 only** (DX12 is listed for all PC tiers). Uses NVIDIA **Streamline** (DLSS/frame-gen path; Streamline calls `Present` directly — relevant for overlay hooks). **Havok** (HKX tagfiles) is present. Confidence: high (DX12/BlackSpace), med (Streamline/Havok, from modder docs).
- **Architecture (critical for co-op):** The single-player exe still contains the **MMO client AND server halves in one process**, talking over an in-memory loopback "virtual session" (`pa::NwVirtualSyncSession` / `pa::NwVirtualAsyncSession`) with ~1,115 opcode-registered "Troc" message handlers (Req/Ack/Nak). Server threads `ServerRunThread`, `ServerWalkThread`, `ServerRepeatTimerThread`; client `ClientRunThread`; all `pa::NetworkThread`. RTTI namespace `pa::`. Confidence: med-high (one detailed public RE write-up, corroborated by a second independent project).
- **Executable:** `<Steam>\steamapps\common\Crimson Desert\bin64\CrimsonDesert.exe` (x64). `crashpad_handler.exe` and `pers.exe` (crash reporter) also present. Confidence: high.
- **Steam App ID:** **3321460** (Deluxe Pack DLC 4024620). Confidence: high.
- **Current version (2026-10-31):** public **patch 2.04.00** (+ same-version hotfix 2026-10-30), released **2026-10-29** with paid DLC "Charting the Unknown". Exe FileVersion **1.0.0.3392** (Steam build **26308950**, with `dlc/dlc_01`, per CDMW research doc reviewed 2026-10-30), followed by hotfix exes **1.0.0.3424** and **1.0.0.3446** (MasterLooter README: "the DLC update and its two October patches"; "The update moved every hooked function"). Latest exe seen: **1.0.0.3446**. Previous widely-targeted modding build: **1.0.0.2976 = patch 2.03.02, Steam build 25477059**. Confidence: high (2.04.00), med (1.0.0.3392 mapping — single source).
- **Patch cadence: very high.** ~1.00.02 (launch 2026-03-19) → 1.18.01 (mid-Aug) → 2.00.00 "Enhanced" (~2026-08-25) → 2.01 … 2.03.02 → 2.04.00 (2026-10-29). Often multiple hotfixes per week early on; roughly every 1–3 weeks since. **Exe FileVersion is an internal build number, not the patch number.** Every patch tends to break offsets; community ASI mods pin to one exe build and disable themselves on others.
- **Archives:** PAZ (data, per-file ChaCha20 + LZ4 raw block) + PAMT (index) in numbered folders (`0000`, `0010`, `0017`…), plus `meta/0.papgt` (group table, has a checksum that overlay writers must update). Data tables are `.pabgb`/`.pabgh`. Saves: `%LOCALAPPDATA%\Pearl Abyss\CD\save`, ChaCha20 + HMAC-SHA256 + LZ4.

## 1. Engine — BlackSpace Engine

### Verified / well-sourced
- In-house engine first used in Black Desert (the BDO engine lineage was remastered/renamed "BlackSpace"); built so PA does not need Unreal/Unity. (search snippets; sports.khan.co.kr 2026-07-07 article on BlackSpace) — confidence high.
- PC requirements list **DirectX 12** for minimum and recommended; Windows 10 64-bit 22H2+, SSD, ~135–150 GB. No Vulkan PC path mentioned anywhere. (snippets: dsogaming, vulkk 2026-03-11) — high.
  - Side evidence: Linux users run it via Proton/vkd3d-proton (DX12→Vulkan translation), and the "RX 580 fix" repos ship a **D3D12 proxy shim** because the game refuses to launch on Polaris GPUs. (GitHub: Vidith007/RX-580-CRIMSON-DESERT-FIX, Megadroidgames/Crimson-Desert-RX-580-Polaris-Fix) — high.
- Rendering features: RT global illumination, ray reconstruction/regeneration, DLSS 4.5 / FSR 4 upscaling (snippets: pcgameshardware tech test). — med.
- Streamline: ByJunter cdmp CONTRIBUTING.md: overlay must use inline hooks on functions, "never vtable patches, because Streamline calls Present directly"; overlay must be installed deferred (after `WorldSystem` exists and window visible) — "Installing it at startup hung the game." — med-high.
- Havok: CDMW research doc decodes HKX tagfiles (Havok SDK version/type registries). — med.
- Shader packages: DXC disassembly of the installed `0017` shader package (DXIL implied). — med.

### Threading / object model (from community RE — treat as med confidence)
- Client/server split in one process (see Key takeaways). Server simulates only actors within the streaming radius of the player; ~125/128 actors matched between client and server sides. Actors share a **u32 actor key at `actor+0x60`** and **serial at `actor+0x50`** between client and server (never pointers). Component pairs: each `Client*Component` has a `Server*Component` counterpart (HP, transform, inventory mirrored). (ByJunter troc-bdo-y-runtime.md, troc-sintesis.md, build 2976, dated 2026-10-03)
- Message frame: `u16 opcode | u8 slot | u16 len | payload`; framing adds 0x1F bytes (header + AES-GCM); encryption disabled in single player. Opcodes **change between builds** → resolve at runtime from the registry/RTTI. Inherited "other player" messages exist: `UpdateVisiblePlayerAck` (2135), `OtherEquipItemAck` (2757), `MoveActorAck` (2697), `RemoveActorAck` (2516) — build 2976 values.
- Hook points proposed (build 2976 RVAs): `ClientNet::PostPacket` 0x35C670, `ClientNet::OnRecvPacket` 0x35CA00; main-thread drain at 0x35D0C0 → 0x35D2A0. Registry hash table at exe+0x6A65BC8. "The disconnect handler is modified by Denuvo."
- Leftover MMO infra in the exe: encrypted login, auth server, "field server", party/guild, World Market, ~116 `PaGamePublic.*` SQL stored procedures, Winsock connect/listen/accept imports. No remote game-server traffic observed (only Steam, Denuvo, error reporting).
- Trinity mod reportedly uses a per-thread "realm" flag in TLS distinguishing client vs server context, with server state overriding client state.
- Quest/stage: retail stubs for cheat Reqs (`CompleteQuestByCheatReq`, `StageChartForceBranchDevReq`); working levers are `StageBranchStateReq` and `QuestDialogEndReq`; "StageBranchStateReq is not validated against the stage's branch table" (bad index crashes client). NPC runtime ids change per load → map by static key. (cranebeaker/coop-anything NOTES.md, 2026-10-19/20, build 2976)
- Player/world chain (older builds, CE-table-derived): `WorldSystem` (RIP-relative singleton) → `+0x30 ActorManager` → `+0x28 UserActor`; position via actor `+0x40 → +0x08 → +0x248` → pos struct `+0x90` (float XYZ), quaternion `+0xA0`; stats component `+0x58`, StatEntry int64 ×1000. Characters: Kliff, Oongka, Damiane. (blizz3010/CrimsonDesertCoop docs/REVERSE_ENGINEERING.md; "May 2026" tables — likely stale on 2.04) — low-med.
- ECS? No source describes a pure ECS. Evidence points to an actor + component model (Client/Server component pairs), not a data-oriented ECS. — med.

## 2. Executable, Steam, launcher

- Path: `steamapps\common\Crimson Desert\bin64\CrimsonDesert.exe` (CDGuardCancel docs; ReShade guides; troubleshooting guides) — high.
- Other processes in bin64: `crashpad_handler.exe` (Ultimate ASI Loader's winmm.dll also gets loaded there — filter by process name in DllMain), `pers.exe` (crash report upload). (ByJunter CONTRIBUTING.md, troc-bdo-y-runtime.md) — med-high.
- Launch: must go through Steam (`steam://rungameid/3321460`); launching the bare exe "spawns and gets killed within a fraction of a second" (CDUMM changelog v3.3.2, 2026-05-11). — med-high.
- Pearl Abyss launcher: **not found** — no evidence of a separate PA launcher in the Steam version; search for "Crimson Desert launcher" returned only generic troubleshooting. Searched: "Crimson Desert launcher", ".exe" guides. — low (absence of evidence).
- Store variants: Microsoft Store/Game Pass exe is a different binary (CDUMM issue #429); macOS and consoles exist. Note the console version numbering differs (1.000.670 = 2.04 on console).
- Steam App ID 3321460 (steambase.io, raijin.gg, CDUMM source `steam://rungameid/3321460`) — high. Deluxe Pack 4024620 (steampulse) — med.

## 3. Version history & cadence (offset-breaking)

| Public patch | Date | Exe FileVersion / Steam build | Source |
|---|---|---|---|
| 1.00.02 day-one | 2026-03-19 | — | gfinityesports (snippet) |
| 1.00.03 | 2026-03-25 | — | tbreak (snippet) |
| 1.01.00 | 2026-03-29 | — | mein-mmo (snippet) |
| 1.0x … 1.12.0 | Apr–mid-Jun | 1.0.0.2850 is an earlier build seen by CDUMM | mp1st, CDUMM #429 |
| 1.15.00 | 2026-07-24 | — | brokenbuilds (snippet) |
| 1.17.00 | 2026-08-07 | — | consolepulse (snippet) |
| 1.18.01 | ~2026-08-15/16 | — | mp1st (snippet) |
| 2.00.00 "Crimson Desert Enhanced" | ~2026-08-25 | — | notebookcheck (snippet) |
| 2.01.00 | ~Sep 2026 | — | official notice boardNo=128 (snippet) |
| ~2.02.00 | — | Steam build 25246367 | ByJunter cheat-tables-versions.md |
| 2.03.00 / 2.03.01 / 2.03.02 | ~Oct 2026 | 1.0.0.2944 / 2949 / **2976** (Steam 25477059) | ByJunter (citing shin2344234 signatures file) |
| **2.04.00** + hotfixes | **2026-10-29** (+10-30) | **1.0.0.3392** (Steam 26308950, dlc/dlc_01) → 1.0.0.3424 → **1.0.0.3446** | keengamer/vulkk/mp1st (snippets); Ratty123/CDMW-Full research doc; shin2344234/master-looter README |

Notes: an "Oct 15 DLC update" is mentioned in cranebeaker notes (unclear). Confidence on exact
FileVersion↔patch mapping: med (community sources).

## 4. Archive & data formats

- **PAZ/PAMT** (verified by multiple independent tool implementations):
  - Numbered group folders (`0000`, `0010`, `0017`, …) each with `0.pamt` index + `N.paz` payloads; `meta/0.papgt` lists mounted groups/order.
  - Per-file **ChaCha20** (RFC 7539), key/nonce derived from Bob Jenkins **lookup3 (hashlittle)** of the lowercase basename with initval `0x000C5EDE`; nonce = seed ×4; key = seed ^ 0x60616263 then per-word deltas. (fitzforge/UltimateCameraMod wiki)
  - Compression: **raw LZ4 block** (no frame header); type in flags bits 16–19. Partial/Sparse DDS special paths.
  - PAMT (build 3392, 2026-10-30): backward varuint name refs + trailing extension dictionary; sparse PAZ IDs; "Legacy readers misread extension bits as encryption" → **2.04 changed the index format detail; old tools may break**. (CDMW-Full research)
  - PAPGT: 4-byte header + 64 opaque trailing bytes; overlay writes must update the legacy PAPGT checksum. Mod managers preserve PAZ timestamps.
- **Data tables:** `.pabgb` (data) / `.pabgh` (header) — NattKh/CrimsonDesertModdingTools ("standalone PABGB parsers… game data schemas"), NattKh/Crimson-Desert-PABG-Editor, NattKh/mod-workbench.
- **Other formats:** PAC/PAM/PAMLOD (sectioned `PAR ` container), PAB (skeleton), PAA (anim), `.paac` (PA Action Chart — animation state machine), PAE/PAEM, PAPR, PALOC (localization), HKX (Havok), `.pbd`, PATHC, PCG, etc.
- **Saves:** `%LOCALAPPDATA%\Pearl Abyss\CD\save`; 128-byte `SAVE` header v2, ChaCha20 nonce @0x1A, HMAC-SHA256 @0x2A, then LZ4 block. Save editors: NattKh, LukeFZ/pycrimson, bbfox0703/CrimsonAtomtic.
- Save warning (2.04): saves made with DLC can't load in non-DLC versions.

## Rumor / unverified
- "Crimson Desert Enhanced" naming of the 2.00 relaunch — multiple snippets agree (med).
- Engine "ECS" — no source. Not found.
- Exact meaning of PAPGT trailer — unknown per CDMW.

## Impossibility claims
- None found for engine/files. (Format-level claims: "Legacy readers misread extension bits as encryption" on build 3392 — a breakage note, not impossibility.)

## Sources (retrieved 2026-10-31)
- https://github.com/fitzforge/UltimateCameraMod/wiki/Dev-PAZ-and-PAMT-Archives — PAZ/PAMT crypto/compression — high
- https://raw.githubusercontent.com/Ratty123/CDMW-Full/main/docs/crimson-desert-research.md — builds 2976/3392, formats, PAPGT, Havok, shaders (reviewed 2026-10-20..30) — med-high
- https://github.com/ByJunter/CrimsonDesertMultiplayer (README; docs/research/troc-sintesis.md; docs/research/troc-bdo-y-runtime.md; docs/research/cheat-tables-versions.md; CONTRIBUTING.md; kit/public/README.txt) — client/server Troc architecture, builds — med-high
- https://raw.githubusercontent.com/cranebeaker/coop-anything/main/games/crimson-desert/NOTES.md — quest/stage sync, .xtls VM, build 2976 (2026-10-19/20) — med
- https://raw.githubusercontent.com/blizz3010/CrimsonDesertCoop/main/docs/REVERSE_ENGINEERING.md — offsets (old builds) — low-med
- https://github.com/faisalkindi/CrimsonDesert-UltimateModsManager (src/cdumm/gui/changelog.py v3.3.2; issue #429) — exe launch behaviour, App ID, builds — med-high
- https://github.com/faisalkindi/CDGuardCancel (HANDOFF_PROMPT.md, HANDOVER_REPORT.md) — exe path, engine — med
- https://steambase.io/games/crimson-desert/info ; https://www.steampulse.org/game/3321460 (snippets) — App ID — high
- https://www.keengamer.com/articles/guides/crimson-desert-patch-2-04-notes-all-charting-the-unknown-dlc-features-and-changes/ ; https://vulkk.com/2026/10/30/crimson-desert-update-2-04-brings-the-dlc-online-fixes-bugs-improves-upscaling-tech/ ; https://www.gamerfuzion.com/crimson-desert-patch-2-04-00-notes-charting-the-unknown-hotfix/ ; https://www.dsogaming.com/patches/crimson-desert-enhanced-update-2-04-00-released-full-patch-notes/ (snippets) — 2.04.00 on 2026-10-29 — high
- https://www.notebookcheck.net/Crimson-Desert-2-0-is-here-with-story-improvements-as-first-sale-goes-live-across-platforms.1378387.0.html (snippet) — 2.00.00 Enhanced — med
- https://consolepulse.com/multiplatform/crimson-desert/guides/crimson-desert-patch-notes-update-tracker ; https://brokenbuilds.gg/crimson-desert/patch-notes/crimson-desert-patch-notes (snippets) — 1.15–1.17 — med
- https://www.dsogaming.com/news/pearl-abyss-has-shared-the-pc-requirements-for-crimson-desert/ ; https://vulkk.com/2026/03/11/pearl-abyss-reveals-crimson-deserts-performance-specs-for-pc-consoles-and-mac/ (snippets) — DX12 — high
- https://sports.khan.co.kr/en/article/202607070606007 (snippet) — BlackSpace Engine — med
- https://www.pcgameshardware.de/Crimson-Desert-Spiel-72378/Tests/Release-Review-Steam-Performance-DLSS-4-5-FSR-4-1521842/2/ (snippet) — RT/upscalers — med
- https://github.com/Vidith007/RX-580-CRIMSON-DESERT-FIX ; https://github.com/Megadroidgames/Crimson-Desert-RX-580-Polaris-Fix — D3D12 proxy shims — high
- https://github.com/NattKh/CrimsonDesertModdingTools — pabgb parsers — med
- https://github.com/shin2344234/master-looter — exe builds 3392/3424/3446 — high
