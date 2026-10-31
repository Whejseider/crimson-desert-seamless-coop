# Comfort Pack (M6) plan

Brief: bigger inventories, better gliding/flying, shorter cooldowns, more, with toggles in one config file.
Status 2026-10-31: plan; nothing built yet. Permissions: docs/mod-permissions.md (verified 2026-10-31).

## How Crimson Desert data mods work (researched 2026-10-31)
- Gameplay values live in the game's data tables (`iteminfo`, inventory, skill, buff, ... `.pabgb`; renamed
  `*.staticinfobody` by the 4 Sep 2026 patch) inside the PAZ archives.
- Mods are applied by a mod manager into an **overlay folder**; the original archives are never modified.
  CDUMM (kindiboy, MIT on GitHub, "ask first" on Nexus) is the current one; DMM and JMM also exist.
- The robust format is **Format 3 `.field.json`**: a `target` table and `intents` that address records by name/key and
  fields by name (`set`, `match`, `clone_record`, `array_append`...). CDUMM verifies every field's byte offset before
  writing and skips what it cannot place, so a game patch does not corrupt anything. (CDUMM docs/FORMAT3_OPS.md.)
- Relevant field names seen in CDUMM's public schema: `_maxStackCount` (items), `_maxSlotCount` /
  `_defaultSlotCount` (inventories), `_coolTime` / `_cooltime` (items, skills), `_sharedCoolTimeGroupNameHash`.
  Glide and traversal stamina sit in buff/skill/action data (Free Gliding and Traversal Unbound edit those).

## Co-op impact
- Inventory size, stack size, storage, camp and item cooldowns are per-player and local: they do not travel over the
  network, so the two players may differ without breaking sync.
- Anything that changes combat numbers (enemy stats, damage, Ultra Hard Mode style buffs) changes how the host's
  enemies behave and must match on both PCs, or be left out. Party scaling (D18) is the co-op answer to difficulty.

## Plan
1. **v1 (now): companions.** The package README links permitted companion mods (link only, never mirrored), installed
   by the player through CDUMM:
   - Fat Stacks (momenaya): stack sizes. Permissive (modify/upload with credit).
   - PrivateStorageAnywherePLUS (Stevi): storage anywhere. Permissive with credit.
   - Infinite Stamina (PhorgeForge): permissive with credit (some assets belong to others).
   - "Ask first" mods (Inventory Expander, Free Gliding, Traversal Unbound, Outfit Loadouts...) are recommended by link
     only, never repackaged.
2. **v2 (after GO, on the user's PC): our own Comfort Pack.** Build `.field.json` files with our own values in CDUMM's
   Game Data tab ("Make mod from edits": edits only verified fields and needs no hex), one file per toggle:
   `CoopPlus-Stacks`, `CoopPlus-Inventory`, `CoopPlus-Cooldowns`, `CoopPlus-Glide`. Ship them in the package's
   `comfort-pack\` folder with a one-click "add to CDUMM" note. Values and toggles are documented in
   `config/comfort.toml` (the single config file the brief asks for); the .field.json files are generated from it.
3. **v3 (later): runtime toggles in the DLL** where a value can be changed in memory safely (needs live RE), so the
   toggles work without a mod manager.

## Open questions for later
- Does CDUMM import a `.field.json` dropped onto its window without a Nexus page? (Its README says drag-and-drop works.)
- Which glide / stamina fields are "verified" in CDUMM's grid on game 1.0.0.3446.
