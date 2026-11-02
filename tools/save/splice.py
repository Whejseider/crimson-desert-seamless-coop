"""Replace a byte range of a decoded save blob and repair what the change moves (test saves only).

Rules (from NattKh's CrimsonSaveEditor parc_inserter3.py, checked on our saves):
- every inline object has 8 x FF then a u32 self pointer = (absolute position of the FF run) + 12;
- an inline object ends with a u32 "tail" = its length - (21 + mask length); objects that contain the change grow;
- the TOC (parc.parse_toc) holds each top-level object's offset and size, preceded by u32 0 | count | stream size.

splice(raw, start, end, new, tails) returns the new blob: [start, end) replaced by `new`, every self pointer after
`start` recomputed, the given tail positions (absolute, in the OLD blob) grown by the size change, the TOC entry
that contains `start` resized, later entries moved, and the stream size adjusted.
"""
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
import parc  # noqa: E402

FF8 = b"\xff" * 8


def self_pointers(raw, lo=0):
    """Absolute positions of FF runs whose following u32 points at itself + 12."""
    out = []
    at = raw.find(FF8, lo)
    while at >= 0:
        if at + 12 <= len(raw) and struct.unpack_from("<I", raw, at + 8)[0] == at + 12:
            out.append(at)
        at = raw.find(FF8, at + 1)
    return out


def element_end(raw, start, mask_len):
    """End of the inline element starting at `start` (u16 mask length first), from its tail rule."""
    for p in range(start + 21 + mask_len, len(raw) - 4):
        if struct.unpack_from("<I", raw, p)[0] == p + 4 - start - (21 + mask_len):
            return p + 4
    raise ValueError(f"no tail for element at 0x{start:X}")


def splice(raw, start, end, new, tails=()):
    raw = bytes(raw)
    growth = len(new) - (end - start)
    types, schema_end = parc.parse_schema(raw)
    toc, _ = parc.parse_toc(raw, schema_end)
    toc_base = schema_end + 12
    ptrs = [p for p in self_pointers(raw, start) if p >= end]
    out = bytearray(raw[:start] + new + raw[end:])
    for p in ptrs:
        struct.pack_into("<I", out, p + growth + 8, p + growth + 12)
    for p in self_pointers(bytes(new)):  # pointers inside the new bytes are relative to their copy
        struct.pack_into("<I", out, start + p + 8, start + p + 12)
    for t in tails:
        assert t < start or t >= end, "a tail inside the replaced range"
        nt = t if t < start else t + growth
        struct.pack_into("<I", out, nt, struct.unpack_from("<I", out, nt)[0] + growth)
    for i, (_cls, _s1, _s2, off, size) in enumerate(toc):
        e = toc_base + i * 20
        if off <= start < off + size:
            struct.pack_into("<I", out, e + 16, size + growth)
        elif off >= end:
            struct.pack_into("<I", out, e + 12, off + growth)
    sp = schema_end + 8
    struct.pack_into("<I", out, sp, struct.unpack_from("<I", out, sp)[0] + growth)
    return bytes(out)
