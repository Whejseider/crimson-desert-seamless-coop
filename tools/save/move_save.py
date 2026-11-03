"""Move a TEST save's player (slot103 test saves only): rewrite TransformFieldSaveData._position.

  python tools/save/move_save.py <in save.save> <out save.save> X Y Z

The offset is the one in build/test-saves/slot103-coop-test-bandits (and saves made from it by replacing fixed-size
fields: dragon/wyvern/egg variants); it is checked against the bandits start (-10349.4, 535.5, -3919.7) first.
"""
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
import cdsave  # noqa: E402

POS = 0x365C8A
header, raw = cdsave.decode(sys.argv[1])
blob = bytearray(raw)
x, y, z = struct.unpack_from("<3f", blob, POS) if POS + 12 <= len(blob) else (0.0, 0.0, 0.0)
if not (abs(x + 10349.4) < 1 and abs(z + 3919.7) < 1):
    # Saves of another size (the dragon variants): the one place holding the bandits start.
    found = [i for i in range(0, len(blob) - 12)
             if abs(struct.unpack_from("<f", blob, i)[0] + 10349.4) < 1
             and abs(struct.unpack_from("<f", blob, i + 8)[0] + 3919.7) < 1]
    assert len(found) == 1, f"start position not found once ({len(found)} candidates)"
    POS = found[0]
    x, y, z = struct.unpack_from("<3f", blob, POS)
print(f"position at 0x{POS:X}")
struct.pack_into("<3f", blob, POS, *map(float, sys.argv[3:6]))
Path(sys.argv[2]).write_bytes(cdsave.encode(bytes(blob), header))
assert cdsave.decode(sys.argv[2])[1] == bytes(blob)
print(f"wrote {sys.argv[2]}: start ({x:.1f}, {y:.1f}, {z:.1f}) -> {sys.argv[3:6]}")
