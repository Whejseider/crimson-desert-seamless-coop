# Crimson Desert save format (for TEST saves only)

Learned 2026-11-02 by reading NattKh's CrimsonSaveEditor (GPL, github.com/NattKh/CRIMSON-DESERT-SAVE-EDITOR-AND-GAME-MODS:
save_crypto.py, save_parser.py, save_pet_rename.py, parc_inserter3.py, gui.py) and checking every rule on our own
saves. Our tools (own code, nothing downloaded is run) are in `tools/save/`. Never write into the user's slots: the test
save is slot103 (build/port3460/testsave.ps1 -Mode In|Out|Swap -Variant <name>), and build/port3479/savecheck.sh must
say SAVES OK after every session.

## Container: `save.save` / `lobby.save`
| Offset | Field |
|---|---|
| 0x00 | "SAVE" |
| 0x04 | u16 version (2) |
| 0x06 | u16 flags (0x0080) |
| 0x12 | u32 uncompressed size |
| 0x16 | u32 payload size |
| 0x1A | 16-byte ChaCha20 nonce (first u32 = block counter) |
| 0x2A | HMAC-SHA256 of the LZ4 block (32 bytes) |
| 0x80 | payload = ChaCha20(LZ4 block(blob)) |

Key = 31 bytes of XOR(base key, "^Pearl--#Abyss__@!!" + "PRIVATE_HMAC_SECRET_CHECK") + a zero byte (tools/save/cdsave.py).
The game loads saves re-encoded by our greedy LZ4 compressor (slightly larger files) without complaint.

## Blob ("PARC" reflection stream)
- `FF FF 04 00 ...`; at 0x0E: u16 tag, u16 0, u16 type count, then the types: name (u32 len + ASCII; the first one
  before the loop), u16 field count, fields (u32 len + name, u32 len + type name, u16 kind, u16 size, u32 aux).
  101 types in our saves (tools/save/parc.py types <blob> [filter]).
- Then u32 0 | u32 object count | u32 stream size | objects x {u32 class, u32 FFFFFFFF, u32 FFFFFFFF, u32 offset,
  u32 size} (TOC). 723 top-level objects in the test save (parc.py toc).
- Inline objects (list elements, nested objects): `u16 mask length | mask | u16 type index | u8 0 | 8 x FF |
  u32 self pointer (= position of the FF run + 12) | u32 0 | fields... | u32 tail`. Fields are present when their mask
  bit is set (list fields are always written). The first field sits 16 bytes after the FF run.
  Tail = element length - (21 + mask length).
- A list: `u8 empty flag` (1 = empty, nothing follows) or `0 | u32 count | u8 0 | i64 0 | i32 0` then the elements.
- Changing a size (tools/save/splice.py): recompute every self pointer after the change, grow the tails of the
  objects that contain it, resize the TOC entry, move later TOC offsets, adjust the stream size.

## Useful records
- MercenaryClanSaveData -> `_mercenaryDataList` of MercenarySaveData: `_characterKey` (u32), `_factionKey`,
  `_mercenaryNo` (u64), `_ownedCharacterKey` (1 = Kliff, 4 = Damian), name, level data, ..., item lists. Mounts, pets
  and hired NPCs live here. The F1 companion wheel shows the current character's own records.
- KnowledgeSaveData -> KnowledgeElementSaveData (first field = knowledge key). Names: NattKh's knowledge_keys_all.json.
- QuestSaveData -> `_questStateList` of QuestStateData: key, delayedFrom, delay/branched/completed times, `_state`
  (5 = completed, 2 = active, 1 = locked/delayed, 3 = other). Mission states: MissionStateData.
- Character keys (NattKh's character_names.json): Kliff 1, Damian 4, Kliff's horse 1003120, Demian's horse 1001173,
  Blackstar dragon Riding_Dragon_1 1000799, Cloudcart Riding_Balloon_Summoner_1 1002041, ATAG mechs 1001984...
- Knowledge for mounts: Knowledge_CallVehicle 1000174, Knowledge_CallDragon 1000175, Knowledge_BlackWolf (Axiom
  Force) 40068, Knowledge_Riding_Balloon_Summoner_1 (Cloudcart) 2147483135; minigames Knowledge_Minigame_* (e.g.
  Arm Wrestling 1002833, Archery Contest 1001651, Duo 1003790, Wrestling Match 1003786).
- Stage keys (NattKh's stage_names.json): minigames are stages, e.g. Her_MiniGame_PubArmwrest 16005100,
  Minigame_ArrowShot 16000200, Hernand_MiniGame_RockPaperSci 1001949, Hernand_MiniGame_Seotda 1000219,
  Her_MiniGame_Duel_Spear 1000302, HernandCastle_Friendly_LegendaryArcheryCompetition 1000804.

## Test-save variants (build/test-saves, local)
- slot103-coop-test-bandits: the base test save (Hernand area, chapter ~2).
- slot103-dragon: Demian's horse record -> Blackstar owned by Kliff, gear removed, Knowledge_CallDragon, the 4 dragon
  quests completed (tools/save/make_dragon_save.py ... wheel). F1 wheel -> Blackstar is summoned and flies.
- slot103-dragon-main: the same on Kliff's main horse record (H then does nothing: not useful).
