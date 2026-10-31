# Third-party mod permissions and credits

Last reviewed: 2026-10-31. Research notes are in `NOTES/RESEARCH/nexus-survey.md`.

The Nexus rows were checked first-hand on 2026-10-31 by reading each mod page's **Permissions and credits** section in the user's signed-in Chrome. They replace the earlier search-snippet summaries. Permissions can change, so re-check a page before shipping anything that depends on it.

Status values: `UNVERIFIED`, `VERIFIED <date>`, `DROPPED <date>`, `ASKED <date>`, `GRANTED <date>`, `DENIED <date>`.

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

Permission codes are from each page's **Permissions and credits** section, read 2026-10-31 in the user's signed-in Chrome.
U = upload or re-host, M = modify, A = use assets, Sold = use in paid mods, DP = use in donation-point files.
"ask" means the page says "You must get permission from me before...".

| Mod | Author | Link | Nexus permissions (2026-10-31) | Status | What we plan to use | How we'll credit |
|---|---|---|---|---|---|---|
| Character Creator - Female and Male | Khione95 | https://www.nexusmods.com/crimsondesert/mods/837 | U no; M ask; A ask; Sold no; DP yes | VERIFIED 2026-10-31 | Nothing bundled. Recommend as companion; compatibility test only | Companion list + compatibility notes |
| Character Creator - Female Armor Module | Khione95 | https://www.nexusmods.com/crimsondesert/mods/3029 | U no; M ask; A ask; Sold no; DP yes | VERIFIED 2026-10-31 | Companion only | Companion list |
| Barber Unlocked | Aerophus | https://www.nexusmods.com/crimsondesert/mods/591 | U no; M ask; A ask; Sold no; DP yes | VERIFIED 2026-10-31 | Companion only | Companion list |
| Barber Unlocked - 2.04 Updated | AlexeySova | https://www.nexusmods.com/crimsondesert/mods/3528 | U yes with credit; M yes (no permission or credit needed); A yes; Sold no; DP no | VERIFIED 2026-10-31 | Companion only (link, no mirror) | Companion list; credit both Aerophus and AlexeySova |
| Outfit Loadouts | lucsOli | https://www.nexusmods.com/crimsondesert/mods/3767 | U no; M ask; A ask; Sold no; DP no | VERIFIED 2026-10-31 | Idea only: our own co-op-aware outfit loadouts | "Inspired by Outfit Loadouts by lucsOli" + companion link |
| Live Transmog (BETA) | tkhquang | https://www.nexusmods.com/crimsondesert/mods/1056 ; https://github.com/tkhquang/CrimsonDesertTools | Nexus: U no; M ask; A ask; Sold no; DP yes. GitHub: MIT | VERIFIED 2026-10-31 (Nexus is stricter than GitHub) | Companion. Policy 5: no code reuse until the author confirms MIT applies | Credit in CREDITS; MIT notice if code is ever used |
| Inventory Expander | kindiboy | https://www.nexusmods.com/crimsondesert/mods/56 | U no; M ask; A ask; Sold no; DP yes | VERIFIED 2026-10-31 | Idea only: our own `inventory_slots` config (our clamp of at most 700) | "Inspired by Inventory Expander by kindiboy" |
| Fat Stacks | momenaya | https://www.nexusmods.com/crimsondesert/mods/157 | U yes with credit; M yes (no permission or credit needed); A yes (no permission or credit needed); Sold no; DP yes | VERIFIED 2026-10-31 | Our own `stack_multiplier`; may adapt its data, with credit anyway | "Based on / inspired by Fat Stacks by momenaya" |
| Free Gliding | Khione95 | https://www.nexusmods.com/crimsondesert/mods/353 | U no; M ask; Conversion no; A ask; Sold no; DP no | VERIFIED 2026-10-31 | Idea only: `glide_stamina_scale` | "Inspired by Free Gliding by Khione95" |
| Free Gliding - Modular glider cost | zbebito | https://www.nexusmods.com/crimsondesert/mods/3300 | U no; M ask; A ask; Sold no; DP no | VERIFIED 2026-10-31 | Idea only | Credit line |
| Infinite Stamina | PhorgeForge | https://www.nexusmods.com/crimsondesert/mods/107 | U yes with credit; M yes with credit; A yes with credit; Sold no; DP no. Page says some assets belong to other authors | VERIFIED 2026-10-31 | Idea only: stamina scales (own values) | "Inspired by Infinite Stamina by PhorgeForge" |
| Traversal Unbound - Stamina reduction | JornB | https://www.nexusmods.com/crimsondesert/mods/3563 | U no; M ask; A ask; Sold no; DP yes | VERIFIED 2026-10-31 | Idea only: split traversal and combat stamina | Credit line |
| Infinite Durability - Cooldown - Stamina Spirit | QuickkNastyy | https://www.nexusmods.com/crimsondesert/mods/2714 | U no; M ask; A ask; Sold no; DP no | VERIFIED 2026-10-31 | Idea only: `item_cooldown_scale` (floor 1 s) | Credit line |
| Ultra Parry and Dodge | Orcax1399 | https://www.nexusmods.com/crimsondesert/mods/607 | U no; M ask; A ask; Sold no; DP yes | VERIFIED 2026-10-31 | Idea only: optional parry assist | Credit line |
| Ultra Parry and Dodge continued | fucojr | https://www.nexusmods.com/crimsondesert/mods/2987 | U no; M ask; A ask; Sold no; DP no | VERIFIED 2026-10-31 | Idea only | Credit line |
| Crimson Desert Teleporter Tool | Dameonk | https://www.nexusmods.com/crimsondesert/mods/713 | U no; M yes with credit; A yes with credit; Sold no; DP no | VERIFIED 2026-10-31 | Idea only: teleport-to-partner | Credit line |
| PrivateStorageAnywherePLUS | Stevi | https://www.nexusmods.com/crimsondesert/mods/388 | U yes with credit; M yes with credit; A yes with credit; Sold no; DP no | VERIFIED 2026-10-31 | Idea only: camp storage hotkey | Credit line |
| Camp Dispatch Overhaul | DesertLink | https://www.nexusmods.com/crimsondesert/mods/3495 | U no; M ask; A ask; Sold no; DP no | VERIFIED 2026-10-31 | Idea only: dispatch timer scale | Credit line |
| Master Looter | shin234 | https://www.nexusmods.com/crimsondesert/mods/3402 | U no; M ask; A ask; Sold no; DP no | VERIFIED 2026-10-31 | Idea only (host-side auto-loot). Nothing copied | "Inspired by Master Looter by shin234" |
| CDAutoLoot | - | https://www.nexusmods.com/crimsondesert/mods/3380 | Page hidden on 2026-09-13 as no longer supported; no permissions shown | DROPPED 2026-10-31 | Nothing | - |
| Classic HUD Continued | Quackers24 (orig. Serfrost, KeithVSmith) | https://www.nexusmods.com/crimsondesert/mods/3489 | U no; M ask; A ask; Sold no; DP no | VERIFIED 2026-10-31 | Companion only | Companion list |
| Minimap Tweaks | Caites | https://www.nexusmods.com/crimsondesert/mods/275 | U no; M ask; A ask; Sold no; DP no | VERIFIED 2026-10-31 | Companion only | Companion list |
| Real infinite Horse Stamina with optional LvL5 Speed | gutzufuss1477 | https://www.nexusmods.com/crimsondesert/mods/2365 | U no; M ask; A ask; Sold no; DP yes | VERIFIED 2026-10-31 | Idea only: `mount_stamina_scale` | Credit line |
| Crimson Desert Visual Redux Reshade | Skytex | https://www.nexusmods.com/crimsondesert/mods/12 | U no (under any circumstances); M ask; A ask; Sold no; DP no | VERIFIED 2026-10-31 | Companion only (link) | Companion list |
| Definitive Mod Manager (DMM) | AerowynX | https://www.nexusmods.com/crimsondesert/mods/633 | U no; M ask; A ask; Sold no; DP no | VERIFIED 2026-10-31 | Supported loader (we document compatibility; we don't bundle it) | Docs mention + link |
| CDUMM | kindiboy / faisalkindi | https://www.nexusmods.com/crimsondesert/mods/207 ; https://github.com/faisalkindi/CrimsonDesert-UltimateModsManager | Nexus: U no; M ask; A ask; Sold no; DP no. GitHub: MIT | VERIFIED 2026-10-31 (Nexus is stricter than GitHub) | Supported loader. Policy 5: no code reuse until the author confirms MIT applies | Docs link |
| JSON Mod Manager (JMM) | PhorgeForge | https://www.nexusmods.com/crimsondesert/mods/113 | U yes with credit; M yes with credit; A yes with credit; Sold no; DP no | VERIFIED 2026-10-31 | Supported (legacy) loader | Docs link |
| Ultimate ASI Loader | ThirteenAG | https://github.com/ThirteenAG/Ultimate-ASI-Loader | GitHub: **MIT**; latest v10.0.2 (2026-10-27) | VERIFIED 2026-10-31 | Loader for our DLL (user installs; we may bundle under MIT with its notice) | Docs link; MIT notice if bundled |

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
