"""Make a TEST save variant by patching values in place (same size, so no table rewrites).

  python tools/save/patch_save.py <in save.save> <out save.save> u32:<blob offset hex>:<old>:<new> [...]

Each patch checks the old value first. The output is re-encoded with cdsave.encode and decoded again to verify.
Only for test saves (build/test-saves/*); never point it at the user's slots.
"""
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
import cdsave  # noqa: E402

FMT = {"u8": "<B", "u16": "<H", "u32": "<I", "u64": "<Q", "f32": "<f"}

src, dst, patches = sys.argv[1], sys.argv[2], sys.argv[3:]
if "Pearl Abyss" in dst and "slot103" not in dst:
    sys.exit("refusing to write into a user save slot")
header, blob = cdsave.decode(src)
blob = bytearray(blob)
for p in patches:
    kind, off, old, new = p.split(":")
    fmt, off = FMT[kind], int(off, 16)
    conv = float if kind == "f32" else (lambda s: int(s, 0))
    cur = struct.unpack_from(fmt, blob, off)[0]
    if cur != conv(old):
        sys.exit(f"0x{off:X}: expected {old}, found {cur}")
    struct.pack_into(fmt, blob, off, conv(new))
    print(f"0x{off:X}: {old} -> {new}")
out = cdsave.encode(bytes(blob), header)
Path(dst).write_bytes(out)
assert cdsave.decode(dst)[1] == bytes(blob)
print(f"wrote {dst} ({len(out)} bytes, verified)")
