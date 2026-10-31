# Research summary (permanent doc; details and sources in NOTES/RESEARCH/)

Last updated 2026-10-31. Confidence noted per item. Nexus, Reddit and most news sites were unreachable from the cloud
session, so those findings come from search snippets (medium confidence); GitHub sources were read directly (high).

## The game
- Pearl Abyss **BlackSpace Engine**, **DX12 only**, NVIDIA Streamline, Havok. Exe `bin64\CrimsonDesert.exe`, Steam AppID
  3321460, also `crashpad_handler.exe`/`pers.exe`. Saves `%LOCALAPPDATA%\Pearl Abyss\CD\save` (encrypted). (high)
- Current: **patch 2.04.00** (2026-10-29, DLC "Charting the Unknown"). Patches every 1-3 weeks; each breaks hardcoded
  addresses. (high/med)
- **The MMO client/server split survives in the single-player exe**: in-memory loopback, ~1,115 "Troc" message types,
  opcode-registered handlers with RTTI names, unencrypted. This is the foundation for co-op. (high, cdmp research)
- Data: PAZ archives (ChaCha20 + LZ4) indexed by PAMT, `meta/0.papgt` checksum; tables `.pabgb/.pabgh`. Modified data
  loads if the indexes/checksums are rewritten; mod managers do this as an overlay. (med-high)

## Protection (brief section 8)
- **Denuvo Anti-Tamper** (online activation; 5 PCs/day limit) and a **Themida-style protector** (`.xtls`, virtualized
  functions, code CRC in some regions). **No anti-cheat** (no EAC/BattlEye/kernel driver) and no bans reported. (med-high)
- DLL/ASI mods load and hook fine for many published mods. Our rules: launch normally via Steam; hook at function entry
  (SafetyHook), outside `.xtls`; no byte patches to protected code; never touch or bypass Denuvo/Themida. (med-high)
- Dev caution: the 5-PCs-a-day activation limit means no rapid VM/machine churn for testing.

## Loading mods
- **Ultimate ASI Loader** (ThirteenAG, MIT) in `bin64` as `winmm.dll`/`dinput8.dll`/`version.dll`; `.asi` files next to
  it. Used by cdmp and most DLL mods. (high)
- Data mods: JSON patch format shared by Definitive Mod Manager (most endorsed), CDUMM (MIT) and JSON Mod Manager. (med)

## Existing co-op work
- **cdmp** (ByJunter, GPL-3, abandoned, invites forks): presence + Steam + overlay + map + experimental shared enemies on
  build 2976. **cd-coop-3446** (nouzert1, GPL-3): port to the newer build, active today. Others: room-code relay fork
  (Nexus 3814), CaptCapybara (shared quests via leftover dev quest commands), blizz3010 (unlicensed, judged
  non-working; must not be copied). u/No-Context2224's Reddit prototype: video only, no code. (high for GitHub)
- Unsolved upstream: stable enemy ids across PCs, enemies targeting the guest, guest loot/XP/quest credit, bosses
  (per-player scripted stages), horse, quests. Leads for each are in `NOTES/CLAIMS.md`.

## Pearl Abyss stance
- Officially single-player. Tried multiplayer internally ("clear graphical sacrifices", 2026-03-27); "far more complex
  than simply adding a new game mode", not in the first DLC (2026-08-11). No official mod tools; mods neither endorsed
  nor blocked. (med) -> keep game-specific code isolated so an official mode can replace the bridge later.

## Lessons from other co-op mods
1. One shared password/lobby; the host's config decides scaling and world rules (ER Seamless Co-op).
2. One owner per entity, ownership migrates on leave; stable ids, never addresses (Skyrim Together).
3. Death or a boss kill never ends the session; respawn at checkpoint (Seamless).
4. Separate co-op saves, backups, clear refusal of mismatched builds.
5. Quest progress advanced by one authority, stage by stage, with duplicate-event guards and a re-sync after loads.

## Mods (Comfort Pack / customization) - see docs/mod-permissions.md
- Confirmed to exist: Character Creator (Khione95), Barber Unlocked (Aerophus; rebuild 3528), Outfit Loadouts
  (lucsOli), Live Transmog (tkhquang, MIT), Inventory Expander (kindiboy), Fat Stacks (momenaya), Free Gliding
  (Khione95), Infinite Stamina (PhorgeForge), Visual Redux ReShade (Skytex), Master Looter, Classic HUD Continued.
- No skill-cooldown or carry-weight mod exists (the game uses slots); fast-travel speed only via load-time mods.
- Plan: reimplement the ideas in our DLL as host-synced values, credit the inspirations, list the rest as companions.
  All Nexus permissions are UNVERIFIED until someone opens each page's Permissions tab.
