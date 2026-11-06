import sys, struct, re
from pe49 import *
IB = 0x140000000
def vtable(mangled):
    out = []
    for m in re.finditer(re.escape(mangled.encode()) + b'\0', d):
        td = rva_of(m.start()) - 0x10
        for m2 in re.finditer(re.escape(struct.pack('<I', td)), d):
            co = m2.start() - 0x0C
            if struct.unpack_from('<II', d, co) != (1, 0): continue
            col = rva_of(co)
            for m3 in re.finditer(re.escape(struct.pack('<Q', IB + col)), d):
                out.append(rva_of(m3.start()) + 8)
    return out
def slot(vt, i): return struct.unpack('<Q', rd(vt + 8 * i, 8))[0] - IB
