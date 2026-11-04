"""Quest sync (offline): give a GUEST save the HOST's quest and story progress, keeping everything else of the guest
(character, position, inventory, equipment, skills, money, knowledge, world). The guest's QuestSaveData object (stage,
mission, quest and quest-gauge states, last completed main quest) is replaced by the host's.

  python tools/save/quest_sync.py <guest save.save> <host save.save> <out save.save> [--check]

How (the save layout notes are in cdsave.py / parc.py / splice.py):
1. Each save carries its own schema (only the types it uses) and objects refer to types by their index in it. Types
   the host's quest object uses that the guest's schema lacks (e.g. QuestGaugeStateData_Stage, which appears later
   in the story) are appended to the guest's schema; everything after the schema moves by that many bytes (TOC
   offsets and every self pointer).
2. In a copy of the host's QuestSaveData, every element's type index is renumbered to the guest's schema and every
   self pointer made relative; splice.splice() then puts it in place of the guest's and repairs the rest.
3. The result is decoded again and checked: same objects in the same order, the guest's other objects byte-identical
   apart from moved pointers, the quest object element-for-element the host's.
The caller makes the backup; this tool never overwrites its inputs.
"""
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
import cdsave  # noqa: E402
import parc  # noqa: E402
import splice  # noqa: E402

FF8 = b"\xff" * 8
QUEST = "QuestSaveData"


def schema_spans(raw):
    """[(name, start, end)] byte spans of each type definition (name included), and the schema end."""
    pos = 0x0E + 6
    count = struct.unpack_from("<H", raw, 0x12)[0]
    spans = []
    for i in range(count):
        start = pos
        n = struct.unpack_from("<I", raw, pos)[0]
        name = raw[pos + 4:pos + 4 + n].decode("ascii")
        pos += 4 + n
        fc = struct.unpack_from("<H", raw, pos)[0]
        pos += 2
        for _ in range(fc):
            for _s in range(2):
                pos += 4 + struct.unpack_from("<I", raw, pos)[0]
            pos += 8
        spans.append((name, start, pos))
    return spans, pos


def object_of(raw, name):
    types, end = parc.parse_schema(raw)
    names = [t[0] for t in types]
    toc, _ = parc.parse_toc(raw, end)
    hits = [(off, size) for cls, _a, _b, off, size in toc if names[cls] == name]
    if len(hits) != 1:
        raise SystemExit(f"{name}: {len(hits)} objects (expected 1)")
    return hits[0]


def elements_in(block, base):
    """(position in block, type index) of every inline element (FF run with its self pointer = base + pos + 12)."""
    out = []
    at = block.find(FF8)
    while at >= 0:
        if at >= 3 and at + 12 <= len(block) and struct.unpack_from("<I", block, at + 8)[0] == base + at + 12:
            out.append((at, struct.unpack_from("<H", block, at - 3)[0]))
        at = block.find(FF8, at + 1)
    return out


def add_types(guest, host, needed):
    """The guest blob with the host's definitions of `needed` type names appended to its schema."""
    gspans, gend = schema_spans(guest)
    hspans, _ = schema_spans(host)
    hdefs = {n: host[s:e] for n, s, e in hspans}
    extra = b"".join(hdefs[n] for n in needed)
    if not extra:
        return guest
    d = len(extra)
    out = bytearray(guest[:gend] + extra + guest[gend:])
    struct.pack_into("<H", out, 0x12, len(gspans) + len(needed))
    # Everything after the schema moved by d: TOC offsets, self pointers.
    types, end = parc.parse_schema(bytes(out))
    assert end == gend + d, "schema did not parse back"
    toc_base = end + 12
    for i, (_cls, _s1, _s2, off, _size) in enumerate(parc.parse_toc(bytes(out), end)[0]):
        struct.pack_into("<I", out, toc_base + i * 20 + 12, off + d)
    for p in splice.self_pointers(guest, gend):
        struct.pack_into("<I", out, p + d + 8, p + d + 12)
    return bytes(out)


def transplant(base, donor, name):
    """`base` with its object `name` replaced by `donor`'s (types remapped / added, pointers rebased)."""
    dtypes, _ = parc.parse_schema(donor)
    dnames = [t[0] for t in dtypes]
    doff, dsize = object_of(donor, name)
    dblock = donor[doff:doff + dsize]
    del_ = elements_in(dblock, doff)
    used = sorted({dnames[ti] for _p, ti in del_})
    bnames = [t[0] for t in parc.parse_schema(base)[0]]
    missing = [n for n in used if n not in bnames]
    base2 = add_types(base, donor, missing)
    bnames2 = [t[0] for t in parc.parse_schema(base2)[0]]
    remap = {dnames.index(n): bnames2.index(n) for n in used}
    block = bytearray(dblock)
    for at, ti in del_:
        struct.pack_into("<H", block, at - 3, remap[ti])
        struct.pack_into("<I", block, at + 8, at + 12)  # relative: splice() makes it absolute again
    boff, bsize = object_of(base2, name)
    return splice.splice(base2, boff, boff + bsize, bytes(block)), missing, len(del_)


def quest_sync(guest, host):
    return transplant(guest, host, QUEST)


def check(guest, host, result):
    """Decodes `result` again and compares it with its sources; raises on any mismatch."""
    gt, ge = parc.parse_schema(guest)
    rt, re_ = parc.parse_schema(result)
    gnames, rnames = [t[0] for t in gt], [t[0] for t in rt]
    assert rnames[:len(gnames)] == gnames, "the guest's types changed order"
    gtoc, rtoc = parc.parse_toc(guest, ge)[0], parc.parse_toc(result, re_)[0]
    assert [gnames[c[0]] for c in gtoc] == [rnames[c[0]] for c in rtoc], "objects differ"
    hoff, hsize = object_of(host, QUEST)
    roff, rsize = object_of(result, QUEST)
    assert rsize == hsize, "quest object size"
    hn = [t[0] for t in parc.parse_schema(host)[0]]
    he = [(p, hn[t]) for p, t in elements_in(host[hoff:hoff + hsize], hoff)]
    rel = [(p, rnames[t]) for p, t in elements_in(result[roff:roff + rsize], roff)]
    assert he == rel, "quest elements differ"
    def norm(b, base):  # an object with its self pointers made relative
        b = bytearray(b)
        for at, _t in elements_in(bytes(b), base):
            struct.pack_into("<I", b, at + 8, 0)
            struct.pack_into("<H", b, at - 3, 0)
        return bytes(b)
    assert norm(host[hoff:hoff + hsize], hoff) == norm(result[roff:roff + rsize], roff), "quest bytes differ"
    for (gc, _a, _b, goff, gsize), (rc, _c, _d, roff2, rsize2) in zip(gtoc, rtoc):
        if gnames[gc] == QUEST:
            continue
        assert gsize == rsize2, f"{gnames[gc]} size changed"
        assert norm(guest[goff:goff + gsize], goff) == norm(result[roff2:roff2 + rsize2], roff2), f"{gnames[gc]} changed"
    return len(rtoc)


if __name__ == "__main__":
    gpath, hpath, out = sys.argv[1:4]
    if Path(out).resolve() in (Path(gpath).resolve(), Path(hpath).resolve()):
        raise SystemExit("the output must be a new file")
    gheader, guest = cdsave.decode(gpath)
    _hh, host = cdsave.decode(hpath)
    result, missing, n = quest_sync(guest, host)
    objects = check(guest, host, result)
    Path(out).write_bytes(cdsave.encode(result, gheader))
    assert cdsave.decode(out)[1] == result, "encode round trip"
    print(f"quest sync: {n} quest elements from the host, types added {missing or 'none'}, {objects} objects checked")
    print(f"wrote {out}")
