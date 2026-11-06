# Imports of the 3549 exe (IAT slot RVA by name) and the `call/jmp [rip+X]` sites that use a slot.
import struct, sys
import numpy as np
import pefile
from pe49 import EXE, d
from xref49 import TB, tva, trs, func_of
_pe = pefile.PE(EXE, fast_load=True)
_pe.parse_data_directories(directories=[pefile.DIRECTORY_ENTRY['IMAGE_DIRECTORY_ENTRY_IMPORT']])
IAT = {}
for e in _pe.DIRECTORY_ENTRY_IMPORT:
    for i in e.imports:
        if i.name: IAT[i.name.decode()] = i.address - _pe.OPTIONAL_HEADER.ImageBase
def iat_calls(slot):
    """FF 15 / FF 25 disp32 (6 bytes) whose target is the IAT slot."""
    b = TB[:trs - 8].astype(np.int64)
    v = b[2:-4] | (b[3:-3] << 8) | (b[4:-2] << 16) | (b[5:-1] << 24)
    v = np.where(v >= 1 << 31, v - (1 << 32), v) + np.arange(len(v)) + 6 + tva
    hits = np.nonzero((v == slot) & (b[:-6] == 0xFF) & ((b[1:-5] == 0x15) | (b[1:-5] == 0x25)))[0]
    return [tva + int(h) for h in hits]
if __name__ == '__main__':
    for n in sys.argv[1:]:
        s = IAT.get(n)
        if s is None: print(n, 'not imported'); continue
        cs = iat_calls(s)
        print(n, hex(s), [(hex(c), hex(func_of(c)[0]) if func_of(c) else None) for c in cs])
