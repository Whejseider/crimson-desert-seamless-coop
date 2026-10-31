# Crimson Desert — Loaders, DLL/ASI Mods, Modding Ecosystem, RE Resources

Retrieved: 2026-10-31. GitHub read directly; Nexus/Reddit blocked (snippets only); FearLess not reachable.

## Key takeaways

- **DLL mods load fine.** Standard route: **Ultimate ASI Loader (UAL, ThirteenAG, x64, v9.7.x)** as a proxy DLL in `...\Crimson Desert\bin64\` next to `CrimsonDesert.exe`, plus `*.asi` plugins (bin64, or `bin64\scripts\` / `bin64\plugins\`). Confidence: **high** (dozens of working mods).
- **Proxy names known to work:** **`winmm.dll`** (ByJunter cdmp kit ships UAL 9.7.4 as winmm.dll; CDUMM bundles/auto-installs `winmm.dll`; MasterLooter default), **`version.dll`** ("known to work on this game" — MasterLooter; used by OptiScaler+ReShade setup), **`dinput8.dll`** (blizz3010 CrimsonDesertCoop, Trinity suggests it), **`winhttp.dll`** (workaround when Win11 KB5095093 broke loading). `dxgi.dll` is taken by ReShade/OptiScaler. Recommendation for our mod: ship UAL as `winmm.dll` by default, document `version.dll`/`winhttp.dll` fallbacks, and coexist with CDUMM/DMM which install their own `winmm.dll`. Confidence: high (winmm/version), med (dinput8/winhttp).
- **Gotcha:** UAL proxy also loads into `crashpad_handler.exe` → filter by host exe name in DllMain (cdmp CONTRIBUTING). Another mod's `winmm.dll` can block all `.asi` loading (MasterLooter README).
- **Hooking approach used by working mods:** AOB **signature scan at load** (no hardcoded RVAs; RIP-relative resolve for globals like `WorldSystem`), **SafetyHook** (inline at function entry; `safetyhook::create_mid` for mid-function sites) — cdmp, CrimsonDesertCoop, CDGuardCancel, Orcax mods, DetourModKit (SafetyHook+Zydis); **MinHook** — Trinity, MasterLooter. DX12 overlay: Dear ImGui via `Present`/`ResizeBuffers`/`ExecuteCommandLists` hooks (Trinity creates a dummy device/swapchain to read vtable; cdmp warns use **inline function hooks, not vtable patches, because Streamline calls Present directly**, and install deferred after WorldSystem exists). Confidence: high.
- **Every patch moves hooked functions.** ASI mods pin exact exe builds and self-disable on mismatch (cdmp: 2976 only; MasterLooter 1.6.52 for 3392/3424/3446; "The update moved every hooked function"). Build a sigcheck tool (MasterLooter `mod\scripts\sigcheck.py` pattern) and per-build manifests (Telemetry `definitions/`).
- **Existing co-op work (most relevant):**
  1. **ByJunter/CrimsonDesertMultiplayer (cdmp)** — GPL-3.0, C++20, UAL winmm.dll + `cdmp.asi`, SafetyHook/Zydis/ImGui, Steam friends P2P; syncs movement, climbing, gliding, combat, horse, gear, rope, time of day, map marker, experimental shared host enemies; spawns the remote player via the engine's own MMO messages (`ActorLoadDataAck`, then replays relayed messages); hooks the two Troc packet functions. Build 2976 only, **unmaintained**. Excellent research docs (Spanish + English index). Best foundation.
  2. **cranebeaker/coop-anything (games/crimson-desert)** — continuing cdmp on build 2976 (Oct 19–20), quest/stage sync via `StageBranchStateReq`/`QuestDialogEndReq` replay.
  3. **blizz3010/CrimsonDesertCoop** — v0.2.3, ASI via UAL dinput8, SafetyHook, Steam `ISteamNetworkingSockets`, companion-hijack approach; ByJunter's audit judges it **non-functional** (mid-function inline hooks, handshake bug, wrong entities, AI-generated). Use only as an offsets map.
  4. Reddit u/No-Context2224 co-op video (~2026-10-06, Notebookcheck/Insider Gaming) — likely cdmp or related; not downloadable.
- **Best RE resources:** ByJunter `docs/research/` (Troc opcode tables, `data/troc_messages_2976.csv`, RTTI, message formats), ZappendusterFX/CrimsonDesertTelemetry (per-build JSON definitions, player/camera chains validated live), shin2344234 mods (per-patch annotated signatures incl. 2.04 builds), tkhquang/CrimsonDesertTools CrimsonDesertCore, XeTrinityz/Trinity `src/game/offsets.h`, bbfox0703/Mydev-Cheat-Engine-Tables `Crimson Desert/` (CT, ~1.17, partly stale), Ratty123/CDMW-Full research doc.
- **FearLess Revolution:** no specific thread found/accessible (searched "fearlessrevolution Crimson Desert cheat table"); CE tables exist on OpenCheatTables (t=1836, not accessed), Nexus (mods 64, 3209 — blocked), vgtimes, gamepressure, gry-online (N3rveMods table 2026-03-20). FearLess is credited as source by CDGuardCancel README ("Cheat Engine findings from the FearLess community").
- **No Lyall "CrimsonDesertFix"** found (GitHub user:Lyall search returned nothing for crimson). Ultrawide is reportedly native; FOV via config/UCM camera mod; no standalone FPS-unlock DLL found.
- **Malware caution:** many GitHub repos named "Kizar Menu", "Trainer 2026", "Save Editor Mod For PC" (e.g., ajayshrma2005-dotcom/KizarCore…, aikushal721/crimson-desert-mod-safety-hub, Speedkrait/…, EndMidwifeArm/Crimson-Desert-X, TVT-Z/…) look like SEO lure repos — do not download binaries.

## 1. Loaders

| Loader / proxy | Evidence | Confidence |
|---|---|---|
| UAL as `winmm.dll` in bin64 | ByJunter kit/public/README.txt ("winmm.dll: the proxy DLL. It is Ultimate ASI Loader v9.7.4"); CDUMM log "ASI loader DLL(s): winmm.dll" + "bundled winmm.dll install"; MasterLooter README | high |
| UAL as `version.dll` | MasterLooter: "version.dll is known to work on this game"; OptiScaler wiki (ReShade as `ReShade.asi` + UAL `version.dll`) | high |
| UAL as `dinput8.dll` | blizz3010 CrimsonDesertCoop README; cranebeaker found bundled file was actually dinput8 | med |
| UAL as `winhttp.dll` | UAL issue #215 user fix after KB5095093 | med |
| Definitive Mod Manager (DMM, Nexus) | installs `.asi` with its own loader (MasterLooter README; NattKh release "updated DMM parser for 1.18") | med |
| CDUMM (faisalkindi) | auto-installs ASI to bin64 with bundled winmm.dll; v3.17.13 (2026-10-28) | high |
| JSON Mod Manager (Nexus) | JSON byte-patch overlay of PAZ data; blizz3010 says it "handles loader setup" | med |
| OptiScaler | `dxgi.dll` or `OptiScaler.asi`; `bin64/plugins/` used by some mods when loaded via OptiScaler | high |
| ReShade (+ addon / RenoDX) | Works (many presets; ReShade with addon support); RenoDX mod "in progress"; Telemetry early versions GPU-hung with ReShade | high |
| Under Proton | `WINEDLLOVERRIDES="winmm=n,b"` (cranebeaker, untested) | low |

Known loader failure: Win11 **KB5095093** (reported 2026-06-29) prevents UAL loading; fixes = uninstall KB, rename to `winhttp.dll`, avoid "Disable fullscreen optimizations" which made Windows load System32 `version.dll`.

## 2. Existing DLL/ASI mods (selection) & their techniques

| Repo | What | Loader | Hook lib | Notes |
|---|---|---|---|---|
| ByJunter/CrimsonDesertMultiplayer | 2-player co-op (alpha) | UAL 9.7.4 `winmm.dll` + `cdmp.asi` | SafetyHook (+Zydis), ImGui | build 1.0.0.2976 / 2.03.02 only; GPL-3.0; tools: peer_sim, host, probe, dx12_test |
| blizz3010/CrimsonDesertCoop | co-op attempt v0.2.3 | UAL `dinput8.dll` + `CrimsonDesertCoop.asi` | SafetyHook mid-hooks | 40+ AOBs in `game_structures.h`; Steam P2P; May-2026 offsets |
| XeTrinityz/Trinity | internal mod menu (DX12) | ASI via UAL (dinput8/winmm/version) | MinHook | `src/game/offsets.h` signatures re-scanned at load; Present/ResizeBuffers/ExecuteCommandLists; XInputGetState detour |
| shin2344234/master-looter (+flight-freedom, stamina-master, bounty-teleportation) | auto-loot etc. | UAL `winmm.dll` (or version.dll) | MinHook; DX12 hook from Trinity | supports exe 1.0.0.3392/3424/3446 (2.04 + Oct hotfixes); `sigcheck.py` |
| tkhquang/CrimsonDesertTools (EquipHide, LiveTransmog, CrimsonDesertCore) | visual mods | ASI | DetourModKit → SafetyHook | updated 2026-10-30; LiveTransmog GUI via ReShade overlay |
| ZappendusterFX/CrimsonDesertTelemetry | read player/camera/lights, local HTTP API 127.0.0.1:27311 | ASI (UAL) | guarded renderer hooks | per-build JSON definitions, exe SHA-256 gate; build 2976 |
| Orcax-1399 player-status-modifier, parry-evade | stat/combat mods | ASI | SafetyHook mid-hooks | archived 2026-09-30 |
| faisalkindi/CDGuardCancel | guard cancel | ASI | SafetyHook + AOB | early Themida findings |
| xsip/CrimsonDesert-Developer-Console-Enabler | re-enables dev console (`ShowDebugConsole`) + custom commands | **external** (no injection) | LiquidHookEx | patches dev-mode disabler; useful for RE |
| Szy-Cathay/CrimsonDesertTweaks, QuickkNastyy/CinematicCombat, DaveTheAve/ShutUpAndLetMePlay, lddurazo23/MountSpeed (2.04.00), La2o/LongerDays, myduoy/AutoLoot-CN | misc ASI | ASI | various | confirm ecosystem active on 2.04 |
| Vidith007/RX-580 fix, Megadroidgames/Polaris fix | D3D12 proxy shim | `d3d12` proxy | — | shows d3d12 proxying works |

## 3. Data-side modding tools (PAZ/PAMT/pabgb)
- faisalkindi/CrimsonDesert-UltimateModsManager (CDUMM) — delta PAZ/PAMT patching, JSON patches, `.cdmod`, ASI, bnk, bsdiff/xdelta; recovery after patches. https://github.com/faisalkindi/CrimsonDesert-UltimateModsManager
- lazorr410/crimson-desert-unpacker — C++17 core + Avalonia GUI + Python CLI; ChaCha20 + LZ4/zlib; repack. (flags>>16&0xF: 0 none, 2 LZ4, 3 custom, 4 zlib)
- fitzforge/UltimateCameraMod — camera via `0010/playercamerapreset.xml`; wiki has PAZ/PAMT spec.
- NattKh: CRIMSON-DESERT-SAVE-EDITOR-AND-GAME-MODS (updated 2026-10-25), CrimsonDesertModdingTools (pabgb parsers/schemas), Crimson-Desert-PABG-Editor, mod-workbench (Rust), CrimsonDesertCommunityItemMapping.
- Ekey/CD.PAZ.Tool, MrIkso/CrimsonDesertTools, LukeFZ/pycrimson, potter420/crimson-rs, hzeemr/crimsonforge (archived), Ratty123/CDMW-Full & CDMW-Lite, Lathiel/Crimson-Desert-JSON-Mod-Manager, Enki013 (macOS JSON MM), yunsuper/ModMerger (offset fixer for JSON mods across updates).
- Caveat: 2.04 (build 3392) changed PAMT details ("Legacy readers misread extension bits as encryption" — CDMW).

## 4. Cheat Engine tables / structs
- bbfox0703/Mydev-Cheat-Engine-Tables → `Crimson Desert/` (CrimsonDesert.CT, GPL-3.0; last ~2026-08-09, ~1.17) — https://github.com/bbfox0703/Mydev-Cheat-Engine-Tables/tree/main/Crimson%20Desert
- OpenCheatTables thread t=1836; Nexus CE tables (mods 64 by bbfox, 3209); vgtimes "Crimson Desert Enhanced — Table" (TalesOfKnighty1815, 2026-07-27); gry-online N3rveMods table (v20.03.2026); gamepressure table (2026-04-06). Not accessed.
- Struct summary (older builds; verify): `WorldSystem` → +0x30 ActorManager → +0x28 UserActor; pos via core+0x248 → +0x90 XYZ float, +0xA0 quat; stats comp actor+0x58 (StatEntry int64 ×1000; type 0=HP,17=Stamina,18=Spirit); actor key u32 @+0x60 (client/server shared), serial @+0x50. Example AOB (blizz3010, old build): camera zoom write `F3 41 0F 11 84 24 D8 00 00 00`; worldOffset `0F ?? ?? ?? ?? ?? ?? 0F 11 ?? 90 00 00 00 E8 ?? ?? ?? ?? F3`. Expect these to be stale on 2.04.
- ByJunter cheat-tables-versions.md concludes most 1.x signatures don't match 2.03.02; Telemetry, tkhquang and shin's work are most reliable.

## 5. Frameworks (ReFramework-like)
- None found equivalent to REFramework/UE4SS. Closest: DetourModKit (generic toolkit), CrimsonDesertCore (tkhquang shared core), Trinity (menu w/ signature table), dev console enabler. No Lua/script runtime framework found (Orcax parry-evade lists Lua as language — not verified as a framework).

## Rumor / unverified
- DMM internals (Nexus only).
- Which proxy name UAL defaults to in the cdmp kit (winmm per README; cranebeaker found dinput8 contents).

## Impossibility claims
- "Renaming the loader to winmm.dll, version.dll, dinput8.dll, dsound.dll, and xinput1_3.dll makes no difference" (UAL #215, 2026-06-29) — caused by Windows KB5095093, not the game; resolved by uninstalling KB / `winhttp.dll`.
- "ASI plugins … don't work on other builds" (CDUMM README) — true per-build pinning, not impossibility.
- See anticheat-drm.md for Themida "hooks never fire" claim (superseded).

## Sources (retrieved 2026-10-31)
- https://github.com/ByJunter/CrimsonDesertMultiplayer (README, kit/public/README.txt, CONTRIBUTING.md, docs/ANALISIS-CrimsonDesertCoop.md, docs/research/*) — high
- https://raw.githubusercontent.com/cranebeaker/coop-anything/main/games/crimson-desert/NOTES.md — med
- https://github.com/blizz3010/CrimsonDesertCoop — med
- https://github.com/XeTrinityz/Trinity — high
- https://github.com/shin2344234/master-looter — high
- https://github.com/tkhquang/CrimsonDesertTools ; https://github.com/tkhquang/DetourModKit — high
- https://github.com/ZappendusterFX/CrimsonDesertTelemetry — high
- https://github.com/Orcax-1399/CrimsonDesert-player-status-modifier — med
- https://github.com/xsip/CrimsonDesert-Developer-Console-Enabler — med
- https://github.com/faisalkindi/CDGuardCancel — med
- https://github.com/faisalkindi/CrimsonDesert-UltimateModsManager (README, issue #429) — high
- https://github.com/ThirteenAG/Ultimate-ASI-Loader (releases; issue #215) — high
- https://github.com/optiscaler/OptiScaler/wiki/Crimson-Desert (edited 2026-08-06) — high
- https://github.com/clshortfuse/renodx/wiki/mods (snippet) — med
- https://github.com/lazorr410/crimson-desert-unpacker ; https://github.com/fitzforge/UltimateCameraMod/wiki/Dev-PAZ-and-PAMT-Archives — high
- https://github.com/NattKh/CRIMSON-DESERT-SAVE-EDITOR-AND-GAME-MODS ; https://github.com/NattKh/CrimsonDesertModdingTools — med
- https://github.com/bbfox0703/Mydev-Cheat-Engine-Tables — med
- https://raw.githubusercontent.com/Ratty123/CDMW-Full/main/docs/crimson-desert-research.md — med
- https://www.notebookcheck.net/Could-Crimson-Desert-get-co-op-Early-tests-look-promising.1417136.0.html ; https://insider-gaming.com/crimson-desert-multiplayer-mod-is-being-tested/ (snippets, ~2026-10-06) — med
- https://www.xmodhub.com/info/?p=19955 (snippet; vendor) — FOV/ultrawide — low
- https://shazoo.ru/2026/03/27/181656/dlia-crimson-desert-vysel-mod-polnostiu-peredelyvaiushhii-sistemu-kamery (snippet) — camera mod — low
- GitHub repo search "crimson desert" via GitHub API (2026-10-31) — inventory
