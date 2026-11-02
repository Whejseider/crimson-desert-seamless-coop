"""Build the "dragon" TEST save from the COOP TEST bandits save (slot103 test save; never a user slot).

  python tools/save/make_dragon_save.py <bandits save.save> <out save.save> [main|wheel]

- wheel (default): Demian's horse record (MercenarySaveData, key 1001173) becomes Blackstar (Riding_Dragon_1,
  1000799) owned by Kliff (1). It shows on the F1 companion wheel; picking it summons the dragon.
- main: Kliff's own horse record (1003120, his main mount) becomes Blackstar, so H calls it (no horse left).
Both remove the record's items (horse gear): gear on the dragon crashed the game (equip slot index -1, 2026-11-02).
Also: Knowledge_CallDragon (1000175) replaces a region discovery (Haunted Hill, 35008); the 4 dragon quests
(1000354, 1000212, 1000305, 1000319) replace 4 Varnia random-spawn quest records, state 5 (completed).
Offsets are for build/test-saves/slot103-coop-test-bandits and are checked before writing.
"""
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
import cdsave  # noqa: E402
import splice  # noqa: E402

ITEM_TYPE = 15  # ItemSaveData
RECORDS = {"wheel": (0xBD52A, 1001173, 4), "main": (0xBCEA6, 1003120, 1)}  # FF run, key, owner


def put(blob, off, fmt, old, new):
    cur = struct.unpack_from(fmt, blob, off)[0]
    assert cur == old, f"0x{off:X}: expected {old}, found {cur}"
    struct.pack_into(fmt, blob, off, new)


def strip_items(raw, sent):
    """Empty the first item list inside the record whose FF run is at `sent` (mask length 6)."""
    rec_start = sent - 11
    rec_end = splice.element_end(raw, rec_start, 6)
    at = raw.find(b"\xff" * 8, sent + 8, rec_end)
    while at >= 0 and struct.unpack_from("<H", raw, at - 3)[0] != ITEM_TYPE:
        at = raw.find(b"\xff" * 8, at + 8, rec_end)
    if at < 0:
        return raw
    ml = struct.unpack_from("<H", raw, at - 3 - 2 - 4)[0]
    first = at - 3 - 2 - ml
    assert struct.unpack_from("<H", raw, first)[0] == ml, "item mask length"
    list_start = first - 18
    count = struct.unpack_from("<I", raw, list_start + 1)[0]
    assert raw[list_start] == 0 and 0 < count < 64, "unexpected list header"
    end = first
    for _ in range(count):
        iml = struct.unpack_from("<H", raw, end)[0]
        end = splice.element_end(raw, end, iml)
    print(f"record 0x{rec_start:X}-0x{rec_end:X}: {count} items removed (0x{list_start:X}-0x{end:X})")
    return splice.splice(raw, list_start, end, b"\x01", tails=[rec_end - 4])


mode = sys.argv[3] if len(sys.argv) > 3 else "wheel"
sent, old_key, old_owner = RECORDS[mode]
header, raw = cdsave.decode(sys.argv[1])
blob = bytearray(raw)
put(blob, sent + 16, "<I", old_key, 1000799)
if old_owner != 1:
    put(blob, sent + 28, "<I", old_owner, 1)
put(blob, 0x27E9C2, "<I", 35008, 1000175)
for key_off, old, new in ((0x27408D, 1001315, 1000354), (0x2740AC, 1001316, 1000212),
                          (0x2740CB, 1001318, 1000305), (0x2740EA, 1000817, 1000319)):
    put(blob, key_off, "<I", old, new)
    put(blob, key_off + 4, "<B", 2, 5)
out = strip_items(bytes(blob), sent)
Path(sys.argv[2]).write_bytes(cdsave.encode(out, header))
assert cdsave.decode(sys.argv[2])[1] == out
print(f"wrote {sys.argv[2]} ({mode}): blob {len(raw)} -> {len(out)} bytes")
