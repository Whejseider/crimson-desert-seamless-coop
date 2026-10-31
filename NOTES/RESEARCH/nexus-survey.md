# Crimson Desert Nexus Mods survey: Comfort Pack + customization

- Date retrieved: 2026-10-31
- Scope: Crimson Desert (Pearl Abyss, PC) mods on Nexus Mods (`nexusmods.com/crimsondesert`), plus GitHub repos of the same tools.
- Overall confidence: **MEDIUM for existence/IDs, LOW for numbers and permissions.** Reasons are under "Method and limits".

## Key takeaways

1. **Nearly every mod we were asked about exists.** Character Creator (Khione95, mods/837), Barber Unlocked (Aerophus, mods/591), Inventory Expander (mods/56), Fat Stacks (mods/157), Free Gliding (mods/353), Infinite Stamina (mods/107), Visual Redux ReShade (Skytex, mods/12) and Outfit Loadouts (lucsOli, mods/3767) all have Nexus pages. We found **no dedicated skill-cooldown mod**: the cooldown mods target item/gear cooldowns. We also found **no carry-weight mod**, and inventory in this game seems to be slot-based. We found **no data-only "quicker fast travel" mod**: what exists are teleport tools, a load-time mod and game-speed mods.
2. **There are three implementation families:**
   - **JSON byte-patch / field patch of `.pabgb` data tables inside PAZ archives.** Examples: `iteminfo.pabgb`, `buffinfo.pabgb`, `dropsetinfo.pabgb`, `gimmickinfo.pabgb`. A mod manager writes these into an **overlay PAZ** and leaves the original archives untouched. Stamina, stacks, inventory, cooldowns, barber unlocks, HUD layouts and dispatch tweaks mostly work this way.
   - **ASI plugins (C++ DLLs).** These are loaded by ThirteenAG's Ultimate ASI Loader as `bin64\winmm.dll`, `dinput8.dll` or `version.dll`. Auto-loot, transmog, outfit loadouts, parry windows, flight, teleport, Character Creator's runtime part and the trainers (Trinity, DesertLink menu) work this way. Several plugins draw their GUI through a ReShade add-on overlay.
   - **ReShade presets** (Visual Redux and others) and **Cheat Engine tables** (mods/64, mods/3209).
3. **Mod managers dominate. Most data mods need one:**
   - **Definitive Mod Manager (DMM)**, mods/633. It is the most endorsed file on the site (~6.1k endorsements, per snippet) and is now the de facto standard. Character Creator, Outfit Loadouts and Classic HUD Continued require it.
   - **CDUMM**, mods/207. GitHub `faisalkindi/CrimsonDesert-UltimateModsManager`, **MIT licence** (read directly from GitHub). It does field-level merging of `.pabgb` tables and per-field conflict detection.
   - **JSON Mod Manager (JMM)**, mods/113, by PhorgeForge. This is the original format and the one the older mods target.
   - JMM, CDUMM and DMM all apply the same JSON byte-patch format.
4. **Game version risk is high right now.** The current game is **2.04.00**, the DLC patch of about 30 Oct 2026. DMM's author note of 30 Oct 2026 says not to update until mods catch up. Most mods we surveyed target 2.03.00/2.03.02. Byte-offset JSON patches break on table layout changes; field-name patches (CDUMM `.field.json`) and pattern-scanning managers survive better.
5. **Co-op implication.** Gameplay data patches must be byte-identical on both machines. These include stamina, cooldowns, stacks, inventory caps, dispatch and loot. **We should not ship third-party data patches.** We should (a) reimplement the toggleable values in our DLL, read from our `config` (TOML) and synced from host to client in the handshake, or (b) detect known JSON/overlay mods and warn on mismatch. Purely cosmetic and client-side mods are safe as companions: ReShade, HUD layout, map icons, transmog, Character Creator *looks* (but check whether the remote peer can see them).
6. **Permissions are mostly restrictive.** Snippets suggest Fat Stacks and the Barber Unlocked 2.03 rebuild (mods/3528) are permissive. Character Creator, Visual Redux, Inventory Expander, Master Looter, Classic HUD Continued, Outfit Loadouts and DMM all appear to require permission for asset reuse or modification. Our default is therefore: **reimplement the idea in our own code, credit the inspiration, and link the original as a companion.** We do not bundle anything.
7. **Top picks are listed at the bottom ("Recommendations").**

## Method and limits

- `www.nexusmods.com` and `api.nexusmods.com` are blocked here (DNS failure). We tried `web.archive.org` once via WebFetch and it was blocked too.
- All Nexus facts come from **web-search result titles, URLs and engine-summarised snippets** restricted to nexusmods.com. Mod IDs come from result URLs and are reliable. Authors, versions, endorsement counts and permission wording come from the engine's summaries of page text. They are second-hand and may be stale or misattributed.
- GitHub pages could be fetched directly. The CDUMM README and the `tkhquang/CrimsonDesertTools` README were read; both are MIT.
- **Every permission in this file is UNVERIFIED** until a person opens the mod's "Permissions and credits" tab on Nexus. Where a snippet described permissions, we record that as "snippet says ..." only.
- We did not obtain reliable "most downloaded" or "newest" rankings, because the Nexus listing pages cannot be read. What we have comes from snippets.
- Third-party re-host sites (XMODhub, VGTimes) also showed up. **Do not download from them.** They are unofficial mirrors, and several authors forbid re-uploads.

## Summary table

Legend for Impl: JSON = JSON byte/field patch of PAZ/pabgb data applied via mod manager; ASI = native DLL plugin via ASI loader; RS = ReShade preset/add-on; CE = Cheat Engine table; Tool = standalone app.
Rec: **INT** = integrate with credit (only with explicit permission), **COMP** = recommend as companion (user installs from Nexus), **RE** = reimplement the idea in our DLL/config.

| # | Mod (Nexus ID) | Author (source) | Impl | Needs manager? | Version / game | Permissions | Rec |
|---|---|---|---|---|---|---|---|
| 1 | Character Creator - Female and Male (837) | Khione95 (snippets + related-mod credits) | JSON/PAZ data + ASI component | Yes, DMM required | v9.1.16 | Snippet: asset use needs permission; no upload elsewhere; modify needs permission. UNVERIFIED | COMP (+ compat test) |
| 2 | Character Creator - Female Armor Module (3029) | (related to 837; author not confirmed) | data | Yes (DMM) | ? | UNVERIFIED | COMP |
| 3 | Barber Unlocked (591) | Aerophus (snippet) | JSON | Yes (JMM-era) | v4.0; "no longer actively maintained" | Snippet: asset use needs permission. UNVERIFIED | COMP |
| 4 | Barber Unlocked - 2.03/2.04 Updated (3528) | AlexeySova (snippet; credits original) | JSON | Yes | 2.03 / 2.04 | Snippet: assets free without permission, upload allowed with credit, no selling. UNVERIFIED | COMP |
| 5 | Outfit Loadouts (3767) | lucsOli (snippet) | ASI + DMM mod | Yes, DMM >= 4.0.0 + ASI loader | v1.3.0, game 2.03.02; uploaded 20 Oct 2026 | Snippet: asset use needs permission; no selling; no DP. UNVERIFIED | RE (co-op aware loadouts) + COMP |
| 6 | Live Transmog - Runtime Armor Appearance Swap (BETA) (1056) | tkhquang (GitHub) | ASI + ReShade overlay GUI (DetourModKit) | ASI loader + ReShade | v0.17.0, game 2.04.00 (GitHub, 30 Oct) | GitHub repo MIT (verified on GitHub); Nexus tab UNVERIFIED | COMP; MIT code can be studied and reused with attribution |
| 7 | Inventory Expander (56) | kindiboy (snippet; same dev as CDUMM `faisalkindi`, likely) | JSON patch, or standalone Python patcher; early versions ASI | CDUMM or standalone | max cap 700 (>732 crashes) | Snippet: no upload; modify and asset use need permission; DP allowed. UNVERIFIED | RE |
| 8 | Fat Stacks (157) | momenaya (snippet + VGTimes) | JSON / file replace (`iteminfo`) | Yes (JSON manager) | v2.00.00; 2x/3x/5x/10x/999999 | Snippet: very permissive (modify, assets without permission, re-upload with credit). UNVERIFIED | RE (simple multiplier; INT possible if verified) |
| 9 | Stack Master (3548) | ? | ASI + INI multiplier | ASI loader | ? | UNVERIFIED | RE (idea) |
| 10 | Free Gliding (353) | Khione95 (snippet) | JSON | Yes (JMM) | v2.3/2.4 (game 1.08.00) | Snippet: no extra notes given. UNVERIFIED | RE |
| 11 | Free Gliding - Modular glider cost (3300) | zbebito (snippet) | JSON | Yes (DMM) | ? | UNVERIFIED | RE |
| 12 | Infinite Stamina (107) | PhorgeForge (snippet) | JSON | Yes (JMM) | 75/50/25/10/inf variants | UNVERIFIED | RE |
| 13 | Traversal Unbound - Stamina reduction (3563) | ? | JSON (likely) | likely | ? | UNVERIFIED | RE (best design reference: traversal only) |
| 14 | Infinite Durability - Cooldown - Stamina Spirit (2714) | ? | JSON (`iteminfo`) | Yes (DMM/CDUMM) | game 1.13.00 | UNVERIFIED | RE |
| 15 | Infinite Durability and No Cooldowns (2887) | ? | JSON (`iteminfo`) | DMM 1.4 | v1.0.2 | UNVERIFIED | RE |
| 16 | Ultra Parry and Dodge (607) / continued (2987) | ? | ASI | ASI loader / manager | 2987: 2.01.00 | UNVERIFIED | RE (optional assist toggle) |
| 17 | Always Parry - DesertLink (3524) | DesertLink (name) | ASI | ASI loader | 2.02.00 | UNVERIFIED | RE |
| 18 | Crimson Desert Teleporter Tool (713) | ? | ASI / tool | ? | 1.04.02 | UNVERIFIED | RE (co-op "join host" teleport) |
| 19 | Crimson FastLoad (3768) | ? | ASI (likely) | ? | v1.0.10 | UNVERIFIED | COMP (test) |
| 20 | Camp Dispatch Overhaul (3495) | ? | JSON | likely DMM | 2.02.00 | UNVERIFIED | RE (host-authoritative) |
| 21 | PrivateStorageAnywherePLUS (388) / Private Storage Master (3521) | ? | ASI | ASI loader | v1.3.0 (388) | UNVERIFIED | RE |
| 22 | DesertLink - Let's Go Camping (Camp Anywhere) (3781) | DesertLink | ASI (external data store) | ASI loader | 2.03.02 | UNVERIFIED | RE (idea) |
| 23 | Master Looter (3402) | shin234 (snippet) | ASI (code-signed) | ASI loader | v1.6.50, 2.03.00 | Snippet: no upload, no modify, assets need permission, no DP. UNVERIFIED | RE (loot is host-authoritative in our design) |
| 24 | CDAutoLoot (3380) | ? | ASI | ASI loader | v1.0.5 | UNVERIFIED | RE |
| 25 | Classic HUD Continued (3489) | Quackers24 (snippet; orig. Serfrost and KeithVSmith) | data/UI patch | Yes (DMM) | 2.03.02 | Snippet: strict (ask first, no upload, no DP). UNVERIFIED | COMP |
| 26 | Minimap Tweaks (275) | ? | data | standalone (snippet) | ? | UNVERIFIED | COMP |
| 27 | Crimson Navigation - Route to Destination (3447) | ? | ? | ? | ? | UNVERIFIED | COMP |
| 28 | Real infinite Horse Stamina with optional LvL5 Speed (2365) | ? | JSON | likely | 2.03.02 | UNVERIFIED | RE |
| 29 | Movement Speed Gains - DMM Presets (3612) | ? | JSON | DMM | ? | UNVERIFIED | RE |
| 30 | Pardon My Mount - Mounts In Towns (3722) | ? | ? | ? | ? | UNVERIFIED | RE (idea) |
| 31 | Crimson Desert Visual Redux Reshade by Skytex (12) | Skytex | ReShade preset | No (ReShade) | v1.4 (post-1.01.00) | Snippet: no upload under any circumstances; modify needs permission. UNVERIFIED | COMP only |
| 32 | Definitive Mod Manager (633) | AerowynX (snippet) | Tool | n/a | updated 28 Oct 2026 | Snippet: asset use needs permission. UNVERIFIED | COMP (supported loader) |
| 33 | CDUMM (207) | kindiboy / GitHub faisalkindi | Tool | n/a | GitHub v3.17.12 (21 Oct 2026) | GitHub: MIT (read). Nexus tab UNVERIFIED | COMP (supported loader) |
| 34 | JSON Mod Manager (113) | PhorgeForge (snippet) | Tool | n/a | V9 era | UNVERIFIED | COMP (legacy) |
| 35 | Ultimate ASI Loader | ThirteenAG (GitHub) | proxy DLL | n/a | - | (GitHub; not checked) | COMP / required for ASI distribution |

## Per-mod notes

### Customization

**Character Creator (mods/837).** Sources: https://www.nexusmods.com/crimsondesert/mods/837 and file-ID URLs `?tab=files&file_id=14772` ("Female and Male") and `file_id=2517` ("Female").
- **What it does:** head presets, body types, face details, skin colours, many more hair and beard styles and colours, and height via a file edit. All of this happens in the game's barber.
- **Requirements:** DMM, plus enabling it in DMM's "ASI Mods" tab, so it has a native component.
- **Versions:** v9.1.16. The older female-only page (mods/793) is hidden as obsolete.
- **Add-ons and tools:** Female Armor Module (mods/3029), Female Wings Patch (mods/2545). Crimson Desert Mod Workbench (CDMW, mods/826) can export Kliff from CC 9.1.16 as FBX.
- **Known limitation (snippet):** makeup and eyelashes are not applied to player characters.
- **Author:** Khione95, confirmed indirectly. Other mods credit "Character Creator mod made by Khione95", and the search engine attributed mod 837 to Khione95. The snippet did not show the author field directly.
- **Co-op concern:** appearance is stored client-side in data and the save. The remote player will see *their own* local data for our character model. We need to test whether a CC-modified Kliff renders correctly on a peer without CC. A likely failure mode is missing assets, which would fall back or crash.
- **Rec: COMP.** Ship a compatibility note and test matrix. Do not bundle; snippet permissions say ask first.

**Barber Unlocked (mods/591), by Aerophus.**
- **What it does:** unlocks all hairstyles and beards for Kliff, Oongka and Damiane, and adds blonde/brown/red colours.
- **Status (snippet):** "no longer actively maintained" by the original author.
- **Compatibility (snippet):** if another mod overwrites its files, or you uninstall it, the barber UI crashes. Revert to a default hairstyle first.
- **Rebuilds:** "Barber Unlocked - 2.03 Updated" / "2.04 Updated" (mods/3528) is a rebuild that snippets attribute to AlexeySova, crediting Aerophus. Snippets describe its permissions as permissive.
- **Barber access:** Early Barber (mods/2219) swaps the Butcher for the Barber, so it is available earlier.
- **Rec: COMP**, pointing to whichever build matches the current game version. Data-only and cosmetic, so it is safe for co-op as long as the remote peer's rendering copes.

**Outfit Loadouts (mods/3767), by lucsOli.**
- **What it does:** 9 saved outfits per character (Kliff, Damiane, Oongka). Ctrl+1-9 saves, Shift+1-9 wears, and a character-wheel "Outfit" submenu is added. It can optionally include weapons and can pull pieces from or return them to private storage.
- **Versions:** v1.3.0. Needs DMM >= 4.0.0 plus an ASI loader. Game 2.03.02.
- **Listing:** uploaded 20 Oct 2026, featured in Mods of the Month.
- **Rec: RE + COMP.** A loadout feature fits the Comfort Pack. In co-op the equip change must replicate, so we would implement our own equip-set call path that also syncs appearance to the peer. The snippet says asset use needs permission, so we must not copy code.

**Live Transmog (mods/1056), by tkhquang.**
- **What it does:** runtime armor appearance swap without stat changes, with a ReShade overlay GUI (item browser, presets, per-slot).
- **Source:** GitHub https://github.com/tkhquang/CrimsonDesertTools is **MIT**. It is C++ ASI on DetourModKit. Release v0.17.0 targets 2.04.00.
- **Forks:** Rumi's Female Armor Auto-Rules (mods/2908) is a fork. HAWT SWAP (mods/237) combines transmog, a character swapper and gear hiding. DMM also has a built-in transmog exporter.
- **Rec: COMP**, and the best **reference implementation** for our appearance sync, because it is MIT. If we reuse code, keep the MIT notice and credit tkhquang. Verify that the Nexus tab doesn't add conflicting terms.

### Comfort / QoL (gameplay values: must match on both machines)

**Inventory Expander (mods/56).**
- **What it does:** raises inventory slot count. The changelog cap is 700, and values above 732 crash the game. Starting slots above 200 reportedly break candle, brazier and lantern interactions.
- **Install:** via CDUMM or a standalone Python patcher (`CDInventoryExpander.bat`). v1.0.0 used an ASI plugin and a Python patcher.
- **Author:** kindiboy (snippet). The CDUMM GitHub owner is `faisalkindi`, likely the same developer, but this is unconfirmed.
- **Variants:** DesertLink Inventory Expansion 2.03.02 (mods/3569), 999 Backpack bag in Shop (mods/250), All Inventory Max Slots (mods/3249), Warehouse Storage Expander (mods/2816), 999-2000 WareHouse Chest Storage (mods/241), Pack Rat (2.04; ID not captured). DMM also has a built-in "Bag Space 700".
- **Rec: RE.** Offer a config `inventory_slots` clamped to at most 700 (our own limit, informed by their crash report). Sync it from the host. Credit kindiboy for the documented safe cap.

**Fat Stacks (mods/157), by momenaya.**
- **What it does:** 2x/3x/5x/10x/999999 stack sizes by editing `iteminfo.pabgb` data. The VGTimes copy mentions backing up `0.papgt` and file replacement.
- **Versions:** v2.00.00.
- **Related:** FAT STACKS ALL IN ONE (mods/1536), Fat Stacks Plus 999999 (mods/3250), WEAPON STACKING (mods/852), Loot Multiplier + Fat Stacks merged (mods/463), Stack Master (mods/3548, ASI + INI multiplier).
- **Conflicts:** Crimson Loot Overhaul (mods/3306) conflicts with anything that edits `dropsetinfo` / `gimmickinfo` / `iteminfo`. Do not combine Stack Master with Fat Stacks.
- **Rec: RE** as a `stack_multiplier` config. The snippet suggests permissive terms, so we could integrate data with credit *only if* the Nexus tab confirms it. Even then, reimplementing in our DLL is better for co-op sync.

**Free Gliding (mods/353), by Khione95.**
- **What it does:** glide with no stamina drain, with an optional 50% version. Works for Damiane and Oongka.
- **Install / versions:** JSON Mod Manager; v2.3/2.4 (game 1.08.00).
- **Related:** Free Gliding - Modular glider cost (mods/3300, zbebito, with reduced-cost presets), CD_BetterGlide (mods/328), and the flight mods: Enhanced Flight (mods/815, ~938 endorsements), Super Glide (mods/3482, ASI), DesertLink True Flight (mods/3536, 2.03.00), Vertical Flight (mods/409, ASI), Fast Smooth Flight (mods/3792), lemonheads - Unlimited Dragon Flying (mods/356, ~911 endorsements).
- **Conflicts:** run only one flight or glide mod at a time.
- **Rec: RE** as `glide_stamina_scale` (0 to 1). Flight mods are out of scope for co-op v1, because a free-flight player desyncs position authority. Treat them as "not supported".

**Infinite Stamina (mods/107), by PhorgeForge.**
- **What it does:** 75/50/25/10%/infinite stamina use for gliding, sprinting, horse, climbing, swimming and aerial skills.
- **Install:** JSON manager.
- **Alternatives:** CrimsonWings Stamina Manager (mods/110, no longer updated), Traversal Unbound (mods/3563, traversal only with combat untouched; a good design model), Stamina Master (mods/3549), Infinite Swimming and Horse Swimming Stamina (mods/2529), Lower Stamina for Glider/Aerial Roll (mods/106).
- **Rec: RE** with separate `stamina_scale.traversal` and `stamina_scale.combat` values, synced from the host.

**Cooldowns.**
- No skill-only cooldown mod was found.
- Infinite Durability - Cooldown - Stamina Spirit (mods/2714): 1 s / 0.1 s cooldown and cost reductions (25/50/75%), game 1.13.00. The author warns that 0.1 s may crash.
- Infinite Durability and No Cooldowns (mods/2887): edits `iteminfo`.
- Thief-gloves no-cooldown mods: mods/194, 1584, 1623.
- Crimson Game Mods Tool Standalone (mods/1292): one-click QoL that includes cooltime=1.
- FasterSkillPoints (mods/1020) is progression, not cooldowns.
- **Rec: RE** as `item_cooldown_scale` with a floor of about 1 s, based on the 2714 author's crash warning.

**Combat tweaks.**
- Ultra Parry and Dodge (mods/607; continued at mods/2987): ASI, widens the parry, precise-dodge and counter windows.
- Always Parry - DesertLink (mods/3524).
- Trinity Easy Parry (mods/3289/3296): 200/375 ms windows.
- LET ME CHEAT (mods/3083): EzParry.
- DesertLink Ultimate Mod Menu (mods/3638).
- Player status modifier (mods/52).
- **Rec: RE** an optional "assist parry window" per player. This is local-only input timing, so it may differ per player as long as damage resolution stays host-authoritative.

**Fast travel / travel time.**
- No dedicated data mod was found.
- Crimson Desert Teleporter Tool (mods/713): map-marker teleport, F8 undo, game 1.04.02.
- Crimson FastLoad (mods/3768): about 50% less teleport load time per listing; v1.0.10.
- JustSkip (mods/468) and Game Speed Multiplier (mods/295) change global game speed. They are **incompatible with co-op**, because time scale must match.
- TimeSkip - Anywhere (mods/3451). LET ME SLEEP removes the sleep/wait cooldown (ID not captured).
- **Rec: RE** a "teleport to partner" and "summon to host" feature (co-op specific). FastLoad could be a COMP after testing.

**Camp.**
- Camp Dispatch Overhaul (mods/3495): 1-member missions, half cost, 24 h cap, game 2.02.00.
- Enhanced Mission Efficiency (mods/387) and Custom Mission Efficiency (mods/1168).
- PrivateStorageAnywherePLUS (mods/388) and Private Storage Anywhere Continued (2.04.00; ID not captured), Private Storage Master (mods/3521).
- DesertLink - Let's Go Camping (mods/3781): camp anywhere, data stored outside saves.
- Camp Happens (mods/3901): survival overhaul.
- **Rec: RE** "camp storage anywhere" (hotkey) and dispatch timer scaling, both host-authoritative. Do not support survival overhauls in co-op v1.

**Weight limits.** No encumbrance/weight mod was found. Snippets indicate the game uses slot counts plus stack sizes, so this request is covered by the Inventory Expander and Fat Stacks ideas. A Nexus forum thread asks for *movement* weight and inertia to be added back; that is unrelated to carry limits.

**Auto-loot.**
- Master Looter (mods/3402), by shin234: ASI, F10/F11, in-game menu, all three characters; v1.6.50, game 2.03.00. Restrictive per snippet.
- CDAutoLoot (mods/3380): v1.0.5. Skips quest items and searches carcasses once per session to avoid dupes.
- Desert Tooling - Autoloot - Gather - Dispatch Rates (mods/3369).
- Loot Everything (mods/3199).
- AutoLoot Reupload (mods/3326): deprecated, 1.18.02 only.
- Configurable AutoLoot and Fast Interact (mods/95): broke after 1.04.02.
- KukuVacuum (mods/3649).
- Pet loot: mods/3479, mods/3735.
- **Only one auto-looter can run**, because they hook the same functions.
- **Rec: RE** inside our host-authoritative loot (`src/sim/loot.*`). Third-party auto-looters will likely fight our loot hooks, so detect them and warn. The CDAutoLoot "carcass once per session" note is a useful dupe-safety lesson.

**Map/UI.**
- Classic HUD Continued (mods/3489), by Quackers24: DMM, 108 presets, minimap position and size, game 2.03.02.
- Classic HUD and UI Layouts (mods/2609).
- Minimap Tweaks (mods/275): ID from snippet text only.
- Crimson Minimap Vision Cone (mods/3485): ReShade add-on.
- Crimson Navigation (mods/3447).
- Icon mods: Enhanced Mini Map Icons V3 (mods/3628), Enhanced Map Icons (mods/855), Realistic Map Icons (mods/3600).
- The Immersive HUD Mod (mods/2633). CD Friendly HUD (mods/3738).
- Party HUD (mods/3646): unknown details. **Check it**, since it may overlap with our co-op partner HUD.
- Crimson Atlas interactive map (mods/3381). Quick Menu Hotkeys (mods/57). InstantInteract (mods/178).
- **Rec: COMP** for layout and icon mods (client-side cosmetic). We RE only a partner marker on the map/minimap.

**Mounts.**
- Real infinite Horse Stamina with optional LvL5 Speed (mods/2365): 2.03.02; the author says it was only statically checked.
- Movement Speed Gains - DMM Presets (mods/3612).
- Saddle Passive Buffs (mods/2936).
- Pardon My Mount - Mounts In Towns (mods/3722).
- MaxBond (mods/3405).
- Wyvern and Blackstar Boost (mods/3590).
- **Rec: RE** `mount_stamina_scale`, synced. Mount speed changes also affect position interpolation (`src/net/interpolation.*`), so both peers must agree.

**Visuals.**
- Crimson Desert Visual Redux Reshade by Skytex (mods/12): v1.4, HDR variant. Uploaded 20 Mar 2026, last update 31 Mar 2026, so it may be dated for 2.x lighting.
- Others: Noir Depthline (Skytex), Ultimate Reshade (mods/472), Cinematic Crimson Desert (mods/3231), DXHex Essentials 2026 (mods/3900), x5 Real Life Reshades (mods/9), UI Passthrough Reshade Addon (mods/487).
- **Rec: COMP only.** The snippet says no upload under any circumstances. ReShade presets are client-local and co-op-safe. Note the ASI-loader filename clash: ReShade and OptiScaler use `dxgi.dll`, and the ASI loader uses `winmm.dll`. Using ASI mods with OptiScaler is covered at mods/634.

### Loaders / tooling

- **DMM (mods/633, by AerowynX per snippet).** Handles JSON byte-patches, textures, file replacements, ASI/DLL plugins and PAZ overlays. Pattern scanning relocates patches after game updates. Offers to install the ASI loader. Has built-in transmog and "Bag Space 700". About 6.1k endorsements (highest seen). Updated 28 Oct 2026. Snippet: asset use needs permission.
- **CDUMM (mods/207), GitHub faisalkindi/CrimsonDesert-UltimateModsManager.** MIT licence, read directly. Accepts byte-patch JSON, field-name JSON (`.field.json`), `.cdmod` packages, DDS/OG_ XML replacement, ASI (bin64, tracked), Wwise `.bnk`, bsdiff/xdelta and script installers. Overlay directory, field-level merge for `.pabgb` tables, per-field conflict detection, preview and rollback. Latest v3.17.12 (21 Oct 2026). A fork exists at ConsciousDissonance/...
- **JSON Mod Manager (mods/113, by PhorgeForge per snippet).** The original byte-patch format. Decompresses the original files, patches them, writes a new overlay registered in the index (`0.papgt`), leaves the PAZ untouched, and skips patches whose expected bytes don't match.
- **Others:**
  - JSON Mod Creator (mods/913): hex editor for PAZ that saves JSON mods.
  - Crimson Desert Mod Workbench (mods/826) and Mod Creator Workbench (mods/2267).
  - Crimson Game Mods Tool Standalone (mods/1292).
  - Crimson Desert Mod Loader (mods/3527): purpose unknown; snippet says assets cannot be used in sold mods.
  - Mod Manager for Linux (mods/2939): snippet says assets are free to use.
  - Vortex extension (site/mods/1746) and MO2 plugins (site/mods/1782, site/mods/1800).
  - Crimson Save Editor (mods/3748). CDKit modkit (ID not captured).
- **GitHub tooling:**
  - CrimsonForge (Rothfeld): MIT per snippet; unpack/repack with checksums.
  - NattKh/CRIMSON-DESERT-SAVE-EDITOR-AND-GAME-MODS: PAZ-overlay `.pabgb` edits.
  - fitzforge/UltimateCameraMod wiki "Dev PAZ and PAMT Archives": format write-up. The UCM Nexus page is mods/438; snippet says assets usable with credit.
- **ASI loader:** ThirteenAG Ultimate ASI Loader as `bin64\winmm.dll`, `dinput8.dll` or `version.dll`. Use only one loader. ASI files go in `bin64` next to `CrimsonDesert.exe`.
- **Trainers / CE:**
  - Trinity: mods/3289, 3252, 3273, 3296.
  - Crimson Desert Enhanced trainer/CE table: mods/3209.
  - Crimson Desert Cheat Table: mods/64.
  - DesertLink Ultimate Mod Menu: mods/3638.
  - All of these change gameplay values at runtime. **Treat them as unsupported in co-op.**

## Popularity snapshot (low confidence)

- **Most endorsed (snippet counts, captured at different times):**
  - DMM: ~6,110-6,191
  - Character Creator: ~3,089
  - Fat Stacks: ~2,681
  - Inventory Expander: ~2,293
  - CDUMM: ~2,266
  - Enhanced Flight: ~938
  - lemonheads Unlimited Dragon Flying: ~911
  - Crimson Weather (mods/632): ~425
- **Most downloaded:** no reliable data. The Nexus "top" page (two-week window) shows Inventory Expander, LET ME SLEEP, Minimap Tweaks, Better Inventory and Trade UI, and Infinite Stamina among the top entries.
- **Newest / recently updated (Oct 2026):**
  - Outfit Loadouts (3767)
  - Live Transmog v0.17.0 (2.04.00)
  - DesertLink Ultimate Mod Menu (3638, updated 30 Oct)
  - CPU Bottleneck and Stuttering Fix (3922, 30 Oct)
  - DXHex ReShade (3900)
  - Special Abyss Gears Infinite Durability (3918)
  - Longer Days (3897)
  - Camp Happens (3901)
  - Pack Rat (2.04; ID not captured)
  - Private Storage Anywhere Continued (2.04.00; ID not captured)
  - CDKit
- **Highlighted mod lists:** articles (Beebom, Driffle, SportsDunia, GamingBolt, Mein-MMO) consistently list Visual Redux, Inventory Expander and Infinite Stamina.

## Recommendations

**Comfort Pack (reimplemented in our DLL, values in `config/`, host-authoritative and synced to the client):**
1. Inventory slots, clamped to at most 700. Inspired by Inventory Expander.
2. Stack-size multiplier. Inspired by Fat Stacks.
3. Traversal stamina scale (glide/climb/swim/mount), separate from combat stamina. Inspired by Free Gliding, Infinite Stamina and Traversal Unbound.
4. Item cooldown scale, with a floor of 1 s. Inspired by mods/2714.
5. Camp storage anywhere, plus dispatch timer scale. Inspired by PrivateStorageAnywherePLUS and Camp Dispatch Overhaul.
6. Co-op teleport to partner or host. Inspired by the Teleporter Tool.
7. Host-side auto-loot with dupe-safe rules. Inspired by CDAutoLoot and Master Looter.
8. Optional per-player parry/dodge assist window. Inspired by Ultra Parry and Dodge.

**Customization:**
- Outfit loadouts reimplemented with co-op appearance sync (idea from Outfit Loadouts).
- Live Transmog recommended as a companion; its MIT code is a reference.
- Character Creator and Barber Unlocked as companions after a peer-rendering test.

**Companions to document (link only, never bundled):** DMM or CDUMM, Ultimate ASI Loader, Visual Redux, Classic HUD Continued, Minimap Tweaks, map-icon mods.

**Unsupported in co-op v1:** game-speed mods, free-flight mods, trainers and CE tables, survival overhauls, and third-party auto-looters.

## Sources (all retrieved 2026-10-31)

Nexus pages (seen as search results only, not opened):
- https://www.nexusmods.com/crimsondesert/mods/837 (Character Creator); /mods/793; /mods/3029; /mods/2545; /mods/826
- https://www.nexusmods.com/crimsondesert/mods/591 (Barber Unlocked); /mods/3528; /mods/2219
- https://www.nexusmods.com/crimsondesert/mods/3767 (Outfit Loadouts); /mods/motm
- https://www.nexusmods.com/crimsondesert/mods/1056 (Live Transmog); /mods/2908; /mods/237
- https://www.nexusmods.com/crimsondesert/mods/56 (Inventory Expander); /mods/3569; /mods/250; /mods/3249; /mods/2816; /mods/241
- https://www.nexusmods.com/crimsondesert/mods/157 (Fat Stacks); /mods/1536; /mods/3250; /mods/852; /mods/463; /mods/3548; /mods/3306
- https://www.nexusmods.com/crimsondesert/mods/353 (Free Gliding); /mods/3300; /mods/3482; /mods/3536; /mods/409; /mods/815; /mods/3792; /mods/328; /mods/356
- https://www.nexusmods.com/crimsondesert/mods/107 (Infinite Stamina); /mods/110; /mods/3563; /mods/3549; /mods/2529; /mods/106; /mods/824
- https://www.nexusmods.com/crimsondesert/mods/2714; /mods/2887; /mods/2649; /mods/1292; /mods/1020
- https://www.nexusmods.com/crimsondesert/mods/607; /mods/2987; /mods/3524; /mods/3083; /mods/3638; /mods/52
- https://www.nexusmods.com/crimsondesert/mods/713; /mods/3768; /mods/468; /mods/295; /mods/3451
- https://www.nexusmods.com/crimsondesert/mods/3495; /mods/387; /mods/1168; /mods/388; /mods/3521; /mods/3781; /mods/3901
- https://www.nexusmods.com/crimsondesert/mods/3402; /mods/3380; /mods/3326; /mods/3199; /mods/95; /mods/3369; /mods/3649
- https://www.nexusmods.com/crimsondesert/mods/3489; /mods/2609; /mods/3485; /mods/3447; /mods/3628; /mods/855; /mods/3600; /mods/2633; /mods/3646
- https://www.nexusmods.com/crimsondesert/mods/2365; /mods/3612; /mods/2936; /mods/3722
- https://www.nexusmods.com/crimsondesert/mods/12 (Visual Redux); /mods/472; /mods/3231; /mods/3900; /mods/634
- https://www.nexusmods.com/crimsondesert/mods/633 (DMM); /mods/207 (CDUMM); /mods/113 (JMM); /mods/913; /mods/366 (Modding Guide); /mods/3527; /mods/2939; /mods/438
- https://www.nexusmods.com/crimsondesert/mods/top ; /mods/newrecently ; https://forums.nexusmods.com/topic/13534160-mod-request-crimson-desert-add-back-character-weight-and-inertia/

GitHub (fetched directly where noted):
- https://github.com/faisalkindi/CrimsonDesert-UltimateModsManager (README fetched; MIT)
- https://github.com/tkhquang/CrimsonDesertTools (README fetched; MIT)
- https://github.com/Rothfeld/crimsonforge (search result only)
- https://github.com/NattKh/CRIMSON-DESERT-SAVE-EDITOR-AND-GAME-MODS (search result only)
- https://github.com/fitzforge/UltimateCameraMod/wiki/Dev-PAZ-and-PAMT-Archives (search result only)

Secondary:
- https://beebom.com/best-crimson-desert-mods/
- https://driffle.com/blog/10-best-crimson-desert-mods/
- https://www.sportsdunia.com/gaming/crimson-desert-best-mods
- https://mein-mmo.de/en/crimson-desert-beste-mods,1559454/
- https://vgtimes.com/games/crimson-desert/files/93297-.html (Fat Stacks mirror; do not use for downloads)
- https://www.xmodhub.com/... (Inventory Expander mirror; do not use)
