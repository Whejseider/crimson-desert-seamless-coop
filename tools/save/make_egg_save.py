"""Build the "wyvern egg" TEST save from the COOP TEST bandits save (slot103 test save; never a user slot).

  python tools/save/make_egg_save.py <bandits save.save> <out save.save>

- Inventory slot 7 (item 22007 x9, a material) becomes 1 x Item_Egg_Wyvern (1004389, live game data 2026-11-02), so the
  egg can be used from the inventory (it should hatch Animal_Baby_Wyvern_1, 1004045, which grows into
  Riding_Wyvern_1000, 1004233).
- Knowledge_Wyvern (1001881) replaces a region discovery (Haunted Hill, 35008), as make_dragon_save.py does.
Offsets are for build/test-saves/slot103-coop-test-bandits and are checked before writing.
"""
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
import cdsave  # noqa: E402

ITEM = 0x1EE31  # first field (_saveVersion) of the ItemSaveData in inventory slot 7


def put(blob, off, fmt, old, new):
    cur = struct.unpack_from(fmt, blob, off)[0]
    assert cur == old, f"0x{off:X}: expected {old}, found {cur}"
    struct.pack_into(fmt, blob, off, new)


header, raw = cdsave.decode(sys.argv[1])
blob = bytearray(raw)
put(blob, ITEM + 12, "<I", 22007, 1004389)  # _itemKey
put(blob, ITEM + 18, "<q", 9, 1)            # _stackCount
put(blob, 0x27E9C2, "<I", 35008, 1001881)
Path(sys.argv[2]).write_bytes(cdsave.encode(bytes(blob), header))
assert cdsave.decode(sys.argv[2])[1] == bytes(blob)
print(f"wrote {sys.argv[2]}: wyvern egg in inventory slot 7, Knowledge_Wyvern")
