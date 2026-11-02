"""List the elements of a list inside one top-level save object, with the blob offset of each element's first u32
field (its key: characterKey for MercenarySaveData, knowledge key for KnowledgeElementSaveData, ...).

An inline element is   u16 mask_len | mask | u16 type index | u8 | 8 x FF | u32 payload offset | ...  and its first
field sits 16 bytes after the FF run (layout from NattKh's CrimsonSaveEditor, gui.py _dragon_knowledge_swap).

  python tools/save/elements.py <blob.bin> <OwnerType> <ElementType>
  e.g. elements.py t103.bin KnowledgeSaveData KnowledgeElementSaveData
"""
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
import parc  # noqa: E402


def elements(raw, owner, elem):
    types, end = parc.parse_schema(raw)
    names = [t[0] for t in types]
    ti = names.index(elem)
    toc, _ = parc.parse_toc(raw, end)
    out = []
    for cls, _s1, _s2, off, size in toc:
        if names[cls] != owner:
            continue
        blk = raw[off:off + size]
        at = blk.find(b"\xff" * 8)
        while at >= 0:
            if at >= 3 and struct.unpack_from("<H", blk, at - 3)[0] == ti and at + 20 <= len(blk):
                out.append((off + at + 16, struct.unpack_from("<I", blk, at + 16)[0]))
            at = blk.find(b"\xff" * 8, at + 8)
    return out


if __name__ == "__main__":
    raw = open(sys.argv[1], "rb").read()
    for off, key in elements(raw, sys.argv[2], sys.argv[3]):
        print(f"0x{off:X} {key}")
