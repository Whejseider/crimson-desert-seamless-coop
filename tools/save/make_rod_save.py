"""Build the "fishing" TEST save from the COOP TEST bandits save (slot103 test save; never a user slot).

  python tools/save/make_rod_save.py <bandits save.save> <out save.save> [item key]

Inventory slot 7 (item 22007 x9, a material) becomes 1 x FishingRod (item 11001, live game data 2026-11-05: the
item table record named "FishingRod"; FishingRod_II 1000639, FishingRod_III 1000640). The bandits save already
knows fishing (Knowledge_Living_FishingRod 1001897). Offsets are for build/test-saves/slot103-coop-test-bandits.
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


key = int(sys.argv[3]) if len(sys.argv) > 3 else 11001
header, raw = cdsave.decode(sys.argv[1])
blob = bytearray(raw)
put(blob, ITEM + 12, "<I", 22007, key)  # _itemKey
put(blob, ITEM + 18, "<q", 9, 1)        # _stackCount
Path(sys.argv[2]).write_bytes(cdsave.encode(bytes(blob), header))
assert cdsave.decode(sys.argv[2])[1] == bytes(blob)
print(f"wrote {sys.argv[2]}: item {key} x1 in inventory slot 7")
