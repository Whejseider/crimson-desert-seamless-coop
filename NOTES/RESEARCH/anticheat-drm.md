# Crimson Desert — DRM, Anti-Tamper, Anti-Cheat, Integrity

Retrieved: 2026-10-31. Many news sites were not fetchable from this environment; those are cited from
search-result snippets "(snippet)". GitHub sources were read directly.

## Key takeaways

- **Denuvo Anti-Tamper: YES** (Steam store DRM notice added ~1 week before the 2026-03-19 launch; "5 different PC within a day machine activation limit"). Requires online activation and periodic revalidation; offline play afterwards. Detect-It-Easy's Denuvo signature lists Crimson Desert as a sample. Still present on the official Steam build as far as found (reports of removal from the official build are unverified and appear to confuse another game). Confidence: **high**.
- **Themida (packer/VM + code CRC): YES, per multiple modders** — the exe has a ~250 MB `.xtls` section (protector VM; `.xtls` is also a Denuvo section-name heuristic in Detect-It-Easy); some functions are single `jmp` into virtualized code. Early-2026 modders reported **code patches in some regions being silently reverted ("Themida CRC")** and MinHook hooks that installed but never fired. Confidence: **med** (consistent community reports, no vendor confirmation).
- **But in-process hooking clearly works today:** many shipped ASI mods hook game functions with **SafetyHook** (inline at function entry + mid-hooks) or **MinHook** and resolve addresses by AOB signature at load (Trinity, MasterLooter, CDGuardCancel final build, cdmp co-op, CrimsonDesertCoop, DetourModKit-based mods). Practical rule from working mods: hook at **function entry / mid-function with mid-hooks, outside `.xtls`**, never byte-patch code, and expect some hot/protected functions to be virtualized and unhookable. Confidence: **high** that hooking works; **med** on exact boundaries.
- **Anti-cheat: NONE found.** No EAC, BattlEye, XIGNCODE, nProtect, or kernel driver reported for Crimson Desert PC; modders explicitly state "no kernel-level anti-cheat" and "no online anti-cheat files were seen". Single-player; no remote game server traffic observed (only Steam, Denuvo, error reporting). Confidence: **high** (multiple independent sources; absence of evidence across many mod projects).
- **Bans: none reported** for single-player mods (secondary source; PA "neither endorses nor actively blocks"). Confidence: med.
- **File integrity:** The game **loads modified PAZ data without an integrity check** (as long as PAMT/PAPGT checksums are rewritten consistently); mod managers preserve PAZ timestamps so **Steam** doesn't flag/redownload. Exe must be launched via Steam (`steam://rungameid/3321460`); bare exe launch is killed within a fraction of a second (Denuvo/Steam bootstrap). Confidence: med-high.
- **Launcher:** No separate Pearl Abyss launcher found for the Steam build. Processes: `CrimsonDesert.exe`, `crashpad_handler.exe`, `pers.exe` (crash reporter). Confidence: med.
- **Activation-limit caution for dev:** Denuvo limit is 5 PCs/day; one co-op dev noted an open question whether a second install path counts as a new machine. Frequent rebuilds of test VMs/machines could hit this.

## Details

### Denuvo
- Steam store page gained "3rd-Party DRM: Denuvo Anti-tamper, 5 different PC within a day machine activation limit" one week before launch, logged by SteamDB. (PCGamesN, OpenCritic, Insider Gaming, pcgames.de — snippets, March 2026)
- Pearl Abyss response: earlier review builds already had Denuvo, so reviewed performance already included it; "we shouldn't worry" about performance. (multiplayer.it snippet)
- Denuvo was bypassed by a cracker ~early Aug 2026 (build from 1.14.00); DSOGaming/3DNews state it was a bypass, Denuvo still runs in the official build. (snippets) — relevant only to note Denuvo is still in the retail exe. We do not use or reference crack material.
- Detect-It-Easy rule `db/PE/protection_Denuvo.5.sg` lists Crimson Desert as a sample; section heuristics include `.xtls`, `.arch`, `.xdata`, `.xpdata`, `.srdata`. (GitHub horsicq/Detect-It-Easy)
- ByJunter RE notes: "The disconnect handler is modified by Denuvo"; their 13 hook signatures were chosen "at function entry and outside `.xtls`". Denuvo requires internet to activate and revalidate periodically, then can play offline (citing PA FAQ/Steam).

### Themida / code CRC
- faisalkindi/CDGuardCancel research (2026-03-20..04-01, March 2026 build): "Protection: Themida + Denuvo on executable"; "Themida blocks ALL code patches in .sdata"; "MinHook … installs (MH_OK) but hooks silently never fire"; "System DLL hooks (kernel32, xinput, d3d12) … (Themida-safe)"; external `WriteProcessMemory` data writes work; injector code re-applies a 1-byte patch "if Themida reverts". Final README of the same mod: it now **hooks the transition evaluator using SafetyHook with AOB scanning** — i.e. the author later found a working hook path.
- CDUMM changelog (v3.3.2, 2026-05-11): "Crimson Desert is Themida plus Denuvo protected, so the bare exe spawns and gets killed within a fraction of a second" (when launched without Steam).
- blizz3010/CrimsonDesertCoop RE doc: "Themida note: CRC protection reverts executable code patches silently. Only system DLL hooks and SafetyHook work".
- cranebeaker/coop-anything (2026-10-19): function at exe+0x2891FB0 is a single jump into `.xtls` (~250 MB, protector VM), "cannot be read with plain disassembly". CDMW research: a remap-resource path jumps from `0x142C6AA30` into `.xtls` at `0x1508E2CF0`.
- Counter-evidence that MinHook works on current builds: shin2344234/master-looter (MinHook, exe 1.0.0.3392/3424/3446) and XeTrinityz/Trinity (MinHook for DX12 + game hooks via `offsets.h` signatures). So the early "MinHook never fires" observation is likely specific to protected/virtualized regions or to the March build, not universal.

### Anti-cheat
- blizz3010/CrimsonDesertCoop README: game uses Denuvo Anti-Tamper and has **no kernel-level anti-cheat**.
- cranebeaker notes: "no online anti-cheat files were seen".
- ByJunter runtime: no open TCP connections during play; only UDP was their own probe on 127.0.0.1; remote traffic limited to Steam, Denuvo, error reporting, possibly HTTP logging/Kafka.
- BDO (the sibling MMO) historically used Themida + XIGNCODE3 (per a 2017 analysis cited by ByJunter) — **no evidence XIGNCODE ships with Crimson Desert**. Searched: "Crimson Desert XIGNCODE", "EAC", "anti-cheat" — not found.

### Integrity / file checks
- CDGuardCancel FINAL_STATUS: PAZ modding — "no integrity check, no crash with clean patches".
- UCM wiki: timestamp preservation keeps Steam/game from detecting modified PAZ files.
- PAPGT (`meta/0.papgt`) carries a checksum that overlay writers must keep consistent (CDMW research; CDUMM). Bad/half-patched tables "crash on splash" (CDUMM changelog).
- Steam "Verify integrity" will restore originals (standard behavior; CDUMM has revert-to-vanilla).
- ASI plugins self-check the exe build (size/hash) and disable on mismatch — this is mod-side, not game-side.
- ZappendusterFX Telemetry validates exe SHA-256 + hook instruction bytes before instrumenting.

### Crash/ban reports with DLL injection
- Crashes reported are mod-caused: inline hooks placed mid-function (ByJunter's analysis of blizz3010 mod: "will likely crash the game"), invalid values written into game state (CDGuardCancel), DX12 overlay installed too early ("hung the game" — cdmp CONTRIBUTING), ReShade + early Telemetry GPU hang at world entry.
- Windows 11 update **KB5095093** (mid-2026) stopped Ultimate ASI Loader from loading in Crimson Desert for some users; also "Disable fullscreen optimizations" made Windows load System32 `version.dll` instead of the local proxy. Workarounds: uninstall KB / rename proxy to `winhttp.dll`. (UAL issue #215)
- **No ban reports found.** Searched: "Crimson Desert mods ban", "terms of service modding". Secondary source (dexerto snippet) says no players banned for single-player mods.

## Rumor / unverified
- "Denuvo removed from official Steam version" (mezha.ua snippet) — appears to confuse IO Interactive's game; **unverified / likely false**.
- Exact extent of Themida-protected code — unknown; only community observations.

## Impossibility claims
| Claim | Source | Status |
|---|---|---|
| "Code hooking → BLOCKED BY THEMIDA… MinHook installs but hooks never fire (CRC protection)" | faisalkindi/CDGuardCancel FINAL_STATUS.md (March 2026 build) | **Superseded**: same author later shipped SafetyHook-based hooks; MinHook-based mods (Trinity, MasterLooter) work on 2.0x builds. Treat as "byte-patching code / some regions are protected", not "hooking impossible". |
| "Themida blocks ALL code patches in .sdata" | CDGuardCancel research doc | Plausible for that section; avoid raw code byte patches. |
| "CRC protection reverts executable code patches silently. Only system DLL hooks and SafetyHook work" | blizz3010 REVERSE_ENGINEERING.md | Partly contradicted (MinHook works for others). |
| Launching bare exe "gets killed within a fraction of a second" | CDUMM changelog | Correct behavior; always launch via Steam. Debugger-attach-at-start likely also problematic (not tested; not found). |
| "can't mod due to Denuvo" | — | Not found as a claim; extensive working DLL/ASI ecosystem disproves it. |

## Sources (retrieved 2026-10-31)
- https://www.pcgamesn.com/crimson-desert/denuvo-on-steam (snippet) — Denuvo on Steam page — high
- https://opencritic.com/news/27880/a-week-before-launch-crimson-deserts-pc-version-has-just-added-controversial-denuvo-drm (snippet) — high
- https://insider-gaming.com/crimson-desert-denuvo-drm/ (snippet) — high
- https://www.pcgames.de/Crimson-Desert-Spiel-72378/News/Denuvo-Kopierschutz-kurz-vor-Launch-bestaetigt-Steam-1521887/ (snippet) — high
- https://multiplayer.it/notizie/autori-crimson-desert-dicono-non-dobbiamo-preoccuparci-denuvo-problemi-prestazionali.html (snippet) — PA response — med
- https://dsogaming.com/news/crimson-deserts-denuvo-protection-has-been-cracked/ ; https://3dnews.ru/1146282 (snippets) — Denuvo still in retail — med
- https://github.com/horsicq/Detect-It-Easy (db/PE/protection_Denuvo.5.sg) — Denuvo detection sample list incl. Crimson Desert — high
- https://github.com/faisalkindi/CDGuardCancel (README, FINAL_STATUS.md, Crimson_Desert_AnimCancel_Research.md, HANDOFF_PROMPT.md) — Themida observations — med
- https://github.com/faisalkindi/CrimsonDesert-UltimateModsManager (src/cdumm/gui/changelog.py; tests/test_launcher_no_silent_bare_exe_fallback.py; issue #429) — Themida+Denuvo, Steam launch — med-high
- https://github.com/blizz3010/CrimsonDesertCoop (README; docs/REVERSE_ENGINEERING.md) — no kernel AC; Themida CRC note — med
- https://github.com/ByJunter/CrimsonDesertMultiplayer (docs/research/troc-sintesis.md, troc-bdo-y-runtime.md, CONTRIBUTING.md) — Denuvo-modified handler, .xtls, network observation — med-high
- https://raw.githubusercontent.com/cranebeaker/coop-anything/main/games/crimson-desert/NOTES.md — .xtls VM, no AC files, activation question — med
- https://raw.githubusercontent.com/Ratty123/CDMW-Full/main/docs/crimson-desert-research.md — .xtls jump — med
- https://github.com/ThirteenAG/Ultimate-ASI-Loader/issues/215 — KB5095093 loader failure, winhttp.dll workaround — high
- https://github.com/shin2344234/master-looter ; https://github.com/XeTrinityz/Trinity — MinHook-based mods working — high
- https://www.dexerto.com/wikis/crimson-desert/does-crimson-desert-have-mod-support (snippet) — no bans — low-med
