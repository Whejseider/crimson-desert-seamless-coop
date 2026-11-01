# Co-op+ port helper: after a game update, re-find every address the mod uses in the NEW exe and
# print old -> new with the method, so offsets_<build>.h can be written in minutes.
#
#   python port_scan.py <new CrimsonDesert.exe> [<repo>/upstream/cd-coop-3446]
#
# Read-only on the exe. What it covers (the methods of docs/research/port-3460.md and port-3479.md):
#   * code with a recorded head (offsets headers, horse_diag.cpp): nearest match to the old address;
#   * vtables: by RTTI class name;
#   * data globals: decoded from the upstream signatures (one match each);
#   * clock flag: the `cmp byte [rip+d],0` target nearest the old value with the most sites;
#   * minimap icon site, item table, UI-mode push, error logger: their own context signatures.
# Not covered (needs the game running): the Troc table range and the opcodes -- run the live probe
# (build/port3479/troc.py pattern) at the title screen.
import os
import re
import struct
import sys
from collections import Counter

exe = sys.argv[1]
src = sys.argv[2] if len(sys.argv) > 2 else os.path.join(os.path.dirname(__file__), '..', '..', 'upstream', 'cd-coop-3446')
d = open(exe, 'rb').read()
pe = struct.unpack_from('<I', d, 0x3C)[0]
nsec = struct.unpack_from('<H', d, pe + 6)[0]
optsz = struct.unpack_from('<H', d, pe + 20)[0]
IB = struct.unpack_from('<Q', d, pe + 24 + 24)[0]
secs = []
for i in range(nsec):
    o = pe + 24 + optsz + i * 40
    vs, va, rs, ro = struct.unpack_from('<IIII', d, o + 8)
    secs.append((d[o:o + 8].rstrip(b'\0').decode('latin1'), va, vs, ro, rs))
print('exe: PE timestamp 0x%08X  SizeOfImage 0x%08X' % (struct.unpack_from('<I', d, pe + 8)[0],
                                                       struct.unpack_from('<I', d, pe + 24 + 56)[0]))


def off(rva):
    for _, va, vs, ro, rs in secs:
        if va <= rva < va + vs and rva - va < rs:
            return ro + rva - va


def rva_of(o):
    for _, va, vs, ro, rs in secs:
        if ro <= o < ro + rs:
            return va + o - ro


def rd(rva, n):
    o = off(rva)
    return d[o:o + n] if o is not None else b''


def find_all(pat):
    rx = re.compile(b''.join(b'.' if t in ('??', '?') else re.escape(bytes([int(t, 16)])) for t in pat.split()), re.S)
    return [rva_of(m.start()) for m in rx.finditer(d)]


def hexs(b):
    return ' '.join('%02X' % x for x in b)


def read_src(rel):
    p = os.path.join(src, rel)
    return open(p, encoding='latin1').read() if os.path.exists(p) else ''


headers = {n: read_src('src/mod/game/' + n) for n in os.listdir(os.path.join(src, 'src/mod/game'))
           if re.match(r'offsets_\d+\.h$', n)}
newest = sorted(headers, key=lambda n: int(re.findall(r'\d+', n)[0]))[::-1]


def hdr(name):
    for h in newest:
        m = re.search(r'inline constexpr \w+ %s = (0x[0-9A-Fa-f]+)' % re.escape(name), headers[h])
        if m:
            return int(m.group(1), 16)


def head(name):
    for h in newest:
        m = re.search(r'%s\[\]\s*=\s*\{([^}]*)\}' % re.escape(name), headers[h])
        if m:
            return [int(x, 16) for x in re.findall(r'0x([0-9A-Fa-f]{2})\b', m.group(1))]


def report(name, old, hits, how):
    hits = sorted(set(hits), key=lambda h: abs(h - old) if old else 0)
    tag = 'OK ' if hits and (len(hits) == 1 or how.startswith('nearest')) else '?? '
    near = '  '.join('%X(%+X)' % (h, h - old) for h in hits[:3]) if old else ' '.join('%X' % h for h in hits[:3])
    print('%s%-26s old %9s  -> %s   [%s]' % (tag, name, '%X' % old if old else '-', near or 'NOT FOUND', how))


print('\n== code with a recorded head (nearest match) ==')
pairs = [('kPointClimbEnter', 'kPointClimbEnterHead'), ('kRideOnEnter', 'kRideEnterHead'),
         ('kRideOffEnter', 'kRideEnterHead'), ('kActionLayerAdvance', 'kActionLayerAdvanceHead'),
         ('kActionLayersAdvance', 'kActionLayersAdvanceHead'), ('kClientPostPacket', 'kClientPostPacketHead'),
         ('kClientOnRecvPacket', 'kClientOnRecvPacketHead'), ('kStatusFlagsChanged', 'kStatusFlagsChangedHead'),
         ('kMovementMethodStep', 'kMovementMethodStepHead'), ('kErrorLogger', 'kErrorLoggerHead')]
for n, h in pairs:
    old, hb = hdr(n), head(h)
    if old and hb:
        report(n, old, find_all(hexs(hb)), 'nearest head match')
for m in re.finditer(r'constexpr Site (k\w+) = \{(0x[0-9A-F]+), \{([^}]*)\}', read_src('src/mod/net/horse_diag.cpp')):
    hb = [int(x, 16) for x in re.findall(r'0x([0-9A-Fa-f]{2})\b', m.group(3))]
    report('horse_diag ' + m.group(1), int(m.group(2), 16), find_all(hexs(hb)), 'nearest head match')

print('\n== vtables by RTTI name ==')


def vtables(name):
    out = []
    for m in re.finditer(re.escape(name.encode()), d):
        td = rva_of(m.start()) - 0x10
        for m2 in re.finditer(re.escape(struct.pack('<I', td)), d):
            co = m2.start() - 0x0C
            if struct.unpack_from('<II', d, co) != (1, 0):
                continue
            for m3 in re.finditer(re.escape(struct.pack('<Q', IB + rva_of(co))), d):
                out.append(rva_of(m3.start()) + 8)
    return out


for n, cls in [('kVtClientActorManager', 'ClientActorManager'), ('kVtControlledActor', 'ClientChildOnlyInGameActor'),
               ('kVtCharacterControl', 'ClientCharacterControlActorComponent'),
               ('kVtStatusComponent', 'ClientStatusActorComponent'), ('kVtWallUpMovementMethod', 'WallUpMovementMethod'),
               ('kVtNormalMovementMethod', 'NormalMovementMethod'),
               ('kVtClientEquipSlotComponent', 'ClientEquipSlotActorComponent')]:
    report(n, hdr(n) or 0, vtables('.?AV%s@pa@@' % cls), 'RTTI')
cc = vtables('.?AVClientCharacterControlActorComponent@pa@@')
if cc:
    s16 = struct.unpack('<Q', rd(cc[0] + 16 * 8, 8))[0] - IB
    print('   CC vtable slot 16 (horse_diag kCcUpdate) = %X' % s16)

print('\n== data globals by signature ==')


def sig(name, pat, disp_at, insn_end):
    old = hdr(name)
    vals = [h + insn_end + struct.unpack('<i', rd(h + disp_at, 4))[0] for h in find_all(pat)]
    report(name, old or 0, vals, 'signature')


sig('kWorldSystemSlot', '48 8B 0D ?? ?? ?? ?? 48 8B 49 30 E8 ?? ?? ?? ?? 84 C0 75 23 49 8B 46 08 48 8B 48 68 C6 44 24 20 00', 3, 7)
sig('kStaticPlayerPos', 'C5 FB 11 3D ?? ?? ?? ?? 44 89 3D ?? ?? ?? ??', 4, 8)
sig('kCameraSlot', '48 8B 05 ?? ?? ?? ?? C5 FB 10 B0 C8 00 00 00 8B 98 D0 00 00 00', 3, 7)
sig('kChunkOrigin', '48 8B 8B ?? ?? 00 00 C5 FB 10 44 24 ?? C4 E3 79 21 44 24 ?? 20 C5 F8 5C 05 ?? ?? ?? ?? C5 F8 11 81 90 00 00 00', 25, 29)
print('   derived: kClientClockBias = WS+0x2A8, kMinimapIconRoot = WS+0x398, kMinimapIconRootAlt = WS+0x3A0')
old_item = hdr('kItemInfoTableSlot')
load_sites = find_all('4C 8B DC 49 89 5B 08 49 89 6B 18 56 57 41 56 48 83 EC 60 8B FA 48 8B 1D')
cands = [s + 0x15 + 7 + struct.unpack('<i', rd(s + 0x15 + 3, 4))[0] for s in load_sites]
best = sorted(zip(cands, load_sites), key=lambda c: abs(c[0] - old_item))[:1] if old_item else []
for v, s in best:
    print('   kItemInfoTableSlot  %X -> %X  (load at kItemInfoTableLoad %X)' % (old_item, v, s + 0x15))
ui = find_all('0F B6 05 ?? ?? ?? ?? 88 04 0A FF 43 08 C6 05 ?? ?? ?? ?? 00 C6 05 ?? ?? ?? ?? 01')
for s in ui:
    print('   kUiModePush candidate %X -> kGameplayInput %X' % (s, s + 7 + struct.unpack('<i', rd(s + 3, 4))[0]))
for h in find_all('48 8D 4C 24 60 E8 ?? ?? ?? ?? 80 7C 24 48 00 75 6F 8B 05'):
    print('   kMinimapIconDecide %X  head %s' % (h + 10, hexs(rd(h + 10, 16))))

print('\n== clock flag (cmp byte [rip+d],0 targets near the old one) ==')
old_clock = hdr('kClientClockGlobal')
nm, va, vs, ro, rs = secs[0]
buf = d[ro:ro + rs]
c = Counter()
for m in re.finditer(b'\x80\x3D', buf):
    i = m.start()
    if i + 7 > len(buf) or buf[i + 6] != 0:
        continue
    t = va + i + 7 + struct.unpack_from('<i', buf, i + 2)[0]
    if old_clock and abs(t - old_clock) < 0x4000:
        c[t] += 1
# Patches move the flag by a few KB at most and it has 10+ sites: the closest busy target first.
for t, n in sorted([x for x in c.items() if x[1] >= 10], key=lambda x: abs(x[0] - old_clock))[:3]:
    print('   %X (%+X): %d sites' % (t, t - old_clock, n))
print('\nThen: Troc range + opcodes live (title screen), verify-port.ps1 retargeted, an echo test in game.')
