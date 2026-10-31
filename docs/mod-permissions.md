# Third-party mod permissions and credits

Last reviewed: 2026-10-31. Research notes are in `NOTES/RESEARCH/nexus-survey.md`.

We could not open Nexus Mods pages directly; nexusmods.com was unreachable from the research environment. The "Snippet" column paraphrases search-engine summaries of each page, so **every Nexus permission below is UNVERIFIED**. Before acting on a row, someone must open the mod's **Permissions and credits** tab, record what it actually says and the date, and change the Status column.

Status values: `UNVERIFIED`, `VERIFIED <date>`, `ASKED <date>`, `GRANTED <date>`, `DENIED <date>`.

## Policy

1. **Never repackage, re-upload or bundle another author's files** (data patches, JSON, ASI/DLLs, presets, textures) unless that author's own permissions explicitly allow it, or the author grants permission in writing. The written grant must be saved under `docs/permissions/`.
2. **"Ask first" mods are skipped** until the author answers. Skipped means we don't copy code, data, offsets taken from their files, or assets. No answer counts as no.
3. **Default approach: reimplement the idea.** We write our own code and our own config values, and credit the mod that inspired the feature in `CREDITS` / README ("Inspired by X by Y, link").
   - Ideas and gameplay concepts are not assets. Copying a specific byte-patch, offset table or tuned values does count as using their file.
4. **Companions are linked, never mirrored.** Our docs point users to the original Nexus page, never to re-host sites (XMODhub, VGTimes and similar).
5. **Open-source code (for example MIT on GitHub)** may be reused under its licence. Keep the copyright notice, record it in `third_party/`, and still credit the author. If the Nexus page states stricter terms, follow the stricter terms until the author clarifies.
6. **Never use another mod's assets in anything paid or behind donations,** if that mod forbids it.
7. **Co-op rule.** Gameplay-changing data patches must match on both machines. We do not ship third-party gameplay patches; we reimplement them as host-synced config.

## Table

| Mod | Author | Link | Version seen | Permission summary | Status | What we plan to use | How we'll credit |
|---|---|---|---|---|---|---|---|
| Character Creator - Female and Male | Khione95 | https://www.nexusmods.com/crimsondesert/mods/837 | 9.1.16 | Snippet: asset use needs permission; no re-upload; modifying needs permission; no use in sold mods; DP allowed | UNVERIFIED - check Nexus Permissions tab | Nothing bundled. Recommend as companion; compatibility test only | Companion list + compatibility notes |
| Character Creator - Female Armor Module | (not confirmed) | https://www.nexusmods.com/crimsondesert/mods/3029 | ? | Not seen | UNVERIFIED - check Nexus Permissions tab | Companion only | Companion list |
| Barber Unlocked | Aerophus | https://www.nexusmods.com/crimsondesert/mods/591 | 4.0 | Snippet: asset use needs permission; DP allowed | UNVERIFIED - check Nexus Permissions tab | Companion only | Companion list |
| Barber Unlocked - 2.03/2.04 Updated | AlexeySova (snippet) | https://www.nexusmods.com/crimsondesert/mods/3528 | 2.04 | Snippet: assets usable without permission or credit; upload allowed with credit; not in sold mods | UNVERIFIED - check Nexus Permissions tab | Companion only (link, no mirror) | Companion list; credit both Aerophus and the rebuild author |
| Outfit Loadouts | lucsOli | https://www.nexusmods.com/crimsondesert/mods/3767 | 1.3.0 | Snippet: asset use needs permission; not in sold mods; no DP | UNVERIFIED - check Nexus Permissions tab | Idea only: our own co-op-aware outfit loadouts | "Inspired by Outfit Loadouts by lucsOli" + companion link |
| Live Transmog (BETA) | tkhquang | https://www.nexusmods.com/crimsondesert/mods/1056 ; https://github.com/tkhquang/CrimsonDesertTools | 0.17.0 (GitHub) | GitHub repo: **MIT** (read 2026-10-31). Nexus tab not seen | GitHub VERIFIED 2026-10-31; Nexus UNVERIFIED | Companion; code may be studied or reused under MIT | MIT notice in `third_party/` if code is used; credit in CREDITS |
| Inventory Expander | kindiboy | https://www.nexusmods.com/crimsondesert/mods/56 | cap 700 | Snippet: no re-upload; modifying and asset use need permission; DP allowed | UNVERIFIED - check Nexus Permissions tab | Idea only: our own `inventory_slots` config (our clamp of at most 700) | "Inspired by Inventory Expander by kindiboy" |
| Fat Stacks | momenaya | https://www.nexusmods.com/crimsondesert/mods/157 | 2.00.00 | Snippet: permissive (modify freely; assets without permission; re-upload with credit) | UNVERIFIED - check Nexus Permissions tab | Idea: our own `stack_multiplier`. Integrate data only after Nexus terms are verified | "Inspired by Fat Stacks by momenaya" |
| Free Gliding | Khione95 | https://www.nexusmods.com/crimsondesert/mods/353 | 2.3/2.4 | Snippet: author gave no notes on file permissions | UNVERIFIED - check Nexus Permissions tab | Idea only: `glide_stamina_scale` | "Inspired by Free Gliding by Khione95" |
| Free Gliding - Modular glider cost | zbebito (snippet) | https://www.nexusmods.com/crimsondesert/mods/3300 | ? | Not seen | UNVERIFIED - check Nexus Permissions tab | Idea only | Credit line |
| Infinite Stamina | PhorgeForge | https://www.nexusmods.com/crimsondesert/mods/107 | ? | Not seen | UNVERIFIED - check Nexus Permissions tab | Idea only: stamina scales | "Inspired by Infinite Stamina by PhorgeForge" |
| Traversal Unbound - Stamina reduction | (not captured) | https://www.nexusmods.com/crimsondesert/mods/3563 | ? | Not seen | UNVERIFIED - check Nexus Permissions tab | Idea only: split traversal and combat stamina | Credit line |
| Infinite Durability - Cooldown - Stamina Spirit | (not captured) | https://www.nexusmods.com/crimsondesert/mods/2714 | game 1.13.00 | Not seen | UNVERIFIED - check Nexus Permissions tab | Idea only: `item_cooldown_scale` (floor 1 s) | Credit line |
| Ultra Parry and Dodge / continued | (not captured) | https://www.nexusmods.com/crimsondesert/mods/607 ; /mods/2987 | 2.01.00 (2987) | Not seen | UNVERIFIED - check Nexus Permissions tab | Idea only: optional parry assist | Credit line |
| Crimson Desert Teleporter Tool | (not captured) | https://www.nexusmods.com/crimsondesert/mods/713 | 1.04.02 | Not seen | UNVERIFIED - check Nexus Permissions tab | Idea only: teleport-to-partner | Credit line |
| PrivateStorageAnywherePLUS | (not captured) | https://www.nexusmods.com/crimsondesert/mods/388 | 1.3.0 | Not seen | UNVERIFIED - check Nexus Permissions tab | Idea only: camp storage hotkey | Credit line |
| Camp Dispatch Overhaul | (not captured) | https://www.nexusmods.com/crimsondesert/mods/3495 | game 2.02.00 | Not seen | UNVERIFIED - check Nexus Permissions tab | Idea only: dispatch timer scale | Credit line |
| Master Looter | shin234 | https://www.nexusmods.com/crimsondesert/mods/3402 | 1.6.50 | Snippet: no re-upload; no modification; asset use needs permission; no DP; no porting | UNVERIFIED - check Nexus Permissions tab | Idea only (host-side auto-loot). Nothing copied | "Inspired by Master Looter by shin234" |
| CDAutoLoot | (not captured) | https://www.nexusmods.com/crimsondesert/mods/3380 | 1.0.5 | Not seen | UNVERIFIED - check Nexus Permissions tab | Idea only (dupe-safety rule) | Credit line |
| Classic HUD Continued | Quackers24 (orig. Serfrost, KeithVSmith) | https://www.nexusmods.com/crimsondesert/mods/3489 | game 2.03.02 | Snippet: ask first to modify or use assets; no re-upload; no DP; original authors' terms also apply | UNVERIFIED - check Nexus Permissions tab | Companion only | Companion list |
| Minimap Tweaks | (not captured) | https://www.nexusmods.com/crimsondesert/mods/275 | ? | Not seen | UNVERIFIED - check Nexus Permissions tab | Companion only | Companion list |
| Real infinite Horse Stamina with optional LvL5 Speed | (not captured) | https://www.nexusmods.com/crimsondesert/mods/2365 | game 2.03.02 | Not seen | UNVERIFIED - check Nexus Permissions tab | Idea only: `mount_stamina_scale` | Credit line |
| Crimson Desert Visual Redux Reshade | Skytex | https://www.nexusmods.com/crimsondesert/mods/12 | 1.4 | Snippet: no re-upload under any circumstances; modifying needs permission; asset use needs permission | UNVERIFIED - check Nexus Permissions tab | Companion only (link) | Companion list |
| Definitive Mod Manager (DMM) | AerowynX (snippet) | https://www.nexusmods.com/crimsondesert/mods/633 | updated 2026-10-28 | Snippet: asset use needs permission | UNVERIFIED - check Nexus Permissions tab | Supported loader (we document compatibility; we don't bundle it) | Docs mention + link |
| CDUMM | kindiboy / faisalkindi | https://www.nexusmods.com/crimsondesert/mods/207 ; https://github.com/faisalkindi/CrimsonDesert-UltimateModsManager | 3.17.12 (GitHub) | GitHub repo: **MIT** (read 2026-10-31). Nexus tab not seen | GitHub VERIFIED 2026-10-31; Nexus UNVERIFIED | Supported loader; may study its PAZ/pabgb handling under MIT | MIT notice if code is used; docs link |
| JSON Mod Manager (JMM) | PhorgeForge | https://www.nexusmods.com/crimsondesert/mods/113 | V9 era | Not seen | UNVERIFIED - check Nexus Permissions tab | Supported (legacy) loader | Docs link |
| Ultimate ASI Loader | ThirteenAG | https://github.com/ThirteenAG/Ultimate-ASI-Loader | - | Licence not checked | UNVERIFIED - check repo licence | Possible loader for our DLL (user installs) | Docs link |

## Not found

- **Skill cooldowns:** no dedicated skill-only cooldown mod was found. The existing cooldown mods target items and gear.
- **Weight limit:** no carry-weight or encumbrance mod was found. The game appears to use slot limits instead.
- **Quicker fast travel:** no data mod for faster fast travel was found. Only teleport tools, a load-time mod (Crimson FastLoad, mods/3768) and game-speed mods exist.


## Code dependencies (GPL-3.0, verified first-hand on GitHub 2026-10-31)
| Project | Author | Link | Version | Permission | What we use | Credit |
|---|---|---|---|---|---|---|
| cdmp (Crimson Desert Multiplayer) | ByJunter | https://github.com/ByJunter/CrimsonDesertMultiplayer | b62a572 (2026-10-10) | GPL-3.0; author invites forks | Via cd-coop-3446 (whole co-op engine) | README, CREDITS.md, Nexus page, in-game About |
| cd-coop-3446 | nouzert1 | https://github.com/nouzert1/cd-coop-3446 | b7ae133 (2026-10-31) | GPL-3.0 | `upstream/cd-coop-3446/` | same |
| blizz3010/CrimsonDesertCoop | blizz3010 | https://github.com/blizz3010/CrimsonDesertCoop | - | **No licence: do not copy any code** | nothing | - |
