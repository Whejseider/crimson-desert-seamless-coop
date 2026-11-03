"""Give a TEST save (slot103 test saves only) a knowledge entry by replacing one it has (same size, in place).

  python tools/save/knowledge_swap.py <in save.save> <out save.save> <old key> <new key> [<old> <new> ...]

Keys from build/ext-save/knowledge_keys_all.json, e.g. Knowledge_Minigame_RockPaperSci 1002618 (the Hernand boy only
offers rock, paper, scissors with it, 2026-11-02), replacing a region discovery such as Haunted Hill 35008.
"""
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
import cdsave  # noqa: E402
import elements  # noqa: E402

header, raw = cdsave.decode(sys.argv[1])
blob = bytearray(raw)
have = {k: off for off, k in elements.elements(raw, "KnowledgeSaveData", "KnowledgeElementSaveData")}
pairs = list(map(int, sys.argv[3:]))
for old, new in zip(pairs[::2], pairs[1::2]):
    assert old in have, f"the save has no knowledge {old}"
    assert new not in have, f"the save already has knowledge {new}"
    struct.pack_into("<I", blob, have[old], new)
    print(f"knowledge {old} -> {new} at 0x{have[old]:X}")
Path(sys.argv[2]).write_bytes(cdsave.encode(bytes(blob), header))
assert cdsave.decode(sys.argv[2])[1] == bytes(blob)
