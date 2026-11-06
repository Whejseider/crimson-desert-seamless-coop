# Code references (RIP-relative lea/mov/call/jmp) to an RVA in the 3549 exe, and the function (.pdata) around an RVA.
import struct, bisect
from pe49 import d, secs, rva_of, off, rd
import pe49
pe = struct.unpack_from('<I', d, 0x3c)[0]
optsz = struct.unpack_from('<H', d, pe + 20)[0]
opt = pe + 24
exc_rva, exc_size = struct.unpack_from('<II', d, opt + 112 + 3 * 8)  # data directory 3 = exception
funcs = []
o = off(exc_rva)
for i in range(exc_size // 12):
    b, e, u = struct.unpack_from('<III', d, o + 12 * i)
    if b: funcs.append((b, e))
funcs.sort()
starts = [f[0] for f in funcs]
def func_of(rva):
    i = bisect.bisect_right(starts, rva) - 1
    return funcs[i] if i >= 0 and funcs[i][0] <= rva < funcs[i][1] else None
import numpy as np
text = [s for s in secs if s[0] == '.xtext'][0]
_, tva, tvs, tro, trs = text[:5]
TB = np.frombuffer(d, dtype=np.uint8, count=trs, offset=tro)
def _disp_plus_i(lo, hi, at):
    # int32 at offsets lo+at .. hi+at, plus the position, as int64
    b = TB[lo + at: hi + at + 3].astype(np.int64)
    v = b[0:hi - lo] | (b[1:hi - lo + 1] << 8) | (b[2:hi - lo + 2] << 16) | (b[3:hi - lo + 3] << 24)
    v = np.where(v >= 1 << 31, v - (1 << 32), v)
    return v + np.arange(lo, hi, dtype=np.int64)
def xrefs(target):
    """RIP-relative references: disp32 at +3 (7-byte REX op modrm) or call/jmp rel32 at +1."""
    out = []
    n = trs - 16
    CH = 1 << 23
    for lo in range(0, n, CH):
        hi = min(n, lo + CH)
        v3 = _disp_plus_i(lo, hi, 3)
        for i in np.nonzero(v3 == target - tva - 7)[0]:
            j = lo + int(i)
            if (TB[j + 2] & 0xC7) == 0x05: out.append(tva + j)
        v1 = _disp_plus_i(lo, hi, 1)
        for i in np.nonzero(v1 == target - tva - 5)[0]:
            j = lo + int(i)
            if TB[j] in (0xE8, 0xE9): out.append(tva + j)
    return out
