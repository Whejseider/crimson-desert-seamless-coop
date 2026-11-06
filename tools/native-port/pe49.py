import struct, sys
EXE = "C:/Users/asher/CrimsonDesert-SeamlessCoop/build/exe-archive/CrimsonDesert-1.0.0.3549.exe"
d = open(EXE, 'rb').read()
pe = struct.unpack_from('<I', d, 0x3c)[0]
nsec = struct.unpack_from('<H', d, pe+6)[0]; optsz = struct.unpack_from('<H', d, pe+20)[0]
sh = pe+24+optsz
secs = []
for i in range(nsec):
    o = sh+i*40
    nm = d[o:o+8].rstrip(b'\0').decode('latin1')
    vs, va, rs, ro = struct.unpack_from('<IIII', d, o+8)
    ch = struct.unpack_from('<I', d, o+36)[0]
    secs.append((nm, va, vs, ro, rs, ch))
def off(rva):
    for nm, va, vs, ro, rs, ch in secs:
        if va <= rva < va+vs:
            w = rva-va
            return ro+w if w < rs else None
def rd(rva, n):
    o = off(rva); return d[o:o+n] if o is not None else b''
def rva_of(o):
    for nm, va, vs, ro, rs, ch in secs:
        if ro <= o < ro+rs: return va+(o-ro)
def find_all(pat, lo=0, hi=None):
    import re
    rx = re.compile(b''.join(b'.' if t in ('??','?') else re.escape(bytes([int(t,16)])) for t in pat.split()), re.S)
    return [rva_of(m.start()) for m in rx.finditer(d, lo, hi or len(d))]
def hx(b): return ' '.join('%02X'%x for x in b)
