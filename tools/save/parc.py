"""Read the decoded save blob (cdsave.py decode): its embedded schema and object table (TOC).

Blob layout (learned from NattKh's CrimsonSaveEditor save_parser.py, re-written here):
  +0x00 FF FF 04 00 ... | +0x0E u16 tag | u16 0 | u16 type_count | then type_count types:
     name (u32 len + ascii; the first one comes before the loop) | u16 field_count | fields:
     u32 len + field name | u32 len + type name | u16 kind | u16 size | u32 aux
     (the next type's name follows each type except the last)
  then u32 0 | u32 toc_count | u32 stream_size | toc_count x {u32 class, u32 s1, u32 s2, u32 data_off, u32 data_size}
Each object block starts with a field-presence bit mask.

  python tools/save/parc.py types <blob.bin> [filter]      -> type names with field lists
  python tools/save/parc.py toc <blob.bin> [filter]        -> objects (class, offset, size)
"""
import struct
import sys


def _u16(b, o):
    return struct.unpack_from("<H", b, o)[0]


def _u32(b, o):
    return struct.unpack_from("<I", b, o)[0]


def _str(b, o):
    n = _u32(b, o)
    return b[o + 4:o + 4 + n].decode("ascii"), o + 4 + n


def parse_schema(raw):
    pos = 0x0E
    tag, zero, count = struct.unpack_from("<HHH", raw, pos)
    pos += 6
    name, pos = _str(raw, pos)
    types = []
    for i in range(count):
        fc = _u16(raw, pos); pos += 2
        fields = []
        for _ in range(fc):
            fname, pos = _str(raw, pos)
            tname, pos = _str(raw, pos)
            kind, size, aux = struct.unpack_from("<HHI", raw, pos); pos += 8
            fields.append((fname, tname, kind, size, aux))
        types.append((name, fields))
        if i != count - 1:
            name, pos = _str(raw, pos)
    return types, pos


def parse_toc(raw, schema_end):
    zero, count, stream = struct.unpack_from("<III", raw, schema_end)
    base = schema_end + 12
    return [struct.unpack_from("<5I", raw, base + i * 20) for i in range(count)], base + count * 20


if __name__ == "__main__":
    cmd, path = sys.argv[1], sys.argv[2]
    flt = sys.argv[3].lower() if len(sys.argv) > 3 else ""
    raw = open(path, "rb").read()
    types, end = parse_schema(raw)
    if cmd == "types":
        for i, (n, fs) in enumerate(types):
            if flt in n.lower() or any(flt in f[0].lower() or flt in f[1].lower() for f in fs):
                print(f"[{i}] {n}")
                for f in fs:
                    print(f"     {f[0]}: {f[1]} kind={f[2]} size={f[3]} aux={f[4]}")
    elif cmd == "toc":
        toc, _ = parse_toc(raw, end)
        print(f"{len(types)} types, {len(toc)} objects, schema ends 0x{end:X}")
        for i, (cls, s1, s2, off, size) in enumerate(toc):
            n = types[cls][0] if cls < len(types) else f"<{cls}>"
            if flt in n.lower():
                print(f"#{i} {n} off=0x{off:X} size={size} s={s1:X},{s2:X}")
