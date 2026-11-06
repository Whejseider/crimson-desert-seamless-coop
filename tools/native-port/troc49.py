# The 3549 exe's Troc message table, statically: the static initializers do `lea reg,[rip+vtable]` then
# `mov [rip+instance],reg` for each Troc instance in [kTrocInstancesBegin, End) (offsets_s2.h); the instance's u16
# opcode (+0x0C) is in the file, the class name comes from the vtable's RTTI (.?AVTrocTr<Name>@pa@@ -> <Name>).
#   python troc49.py   -> build/native-port/troc_3549.csv (name, id_3549, id_2976) and a summary
import csv, struct, re
from pe49 import rd, d, secs
IB = 0x140000000
BEGIN, END = 0x6E8ED10, 0x6EC2378
tx = [s for s in secs if s[0] == '.xtext'][0]
LO, HI, RVA = tx[3], tx[3] + tx[4], tx[1]

def rtti_name(vt):
    try:
        col = struct.unpack('<Q', rd(vt - 8, 8))[0] - IB
        td = struct.unpack('<I', rd(col + 12, 4))[0]
        raw = rd(td + 0x10, 200).split(b'\0')[0].decode('ascii', 'replace')
    except Exception:
        return None
    m = re.match(r'\.\?AVTrocTr(\w+?)@pa@@', raw)
    return m.group(1) if m else None

def table():
    out = {}
    pat = re.compile(rb'\x48\x8D([\x05\x0D\x15\x1D\x35\x3D])(....)\x48\x89(\1)(....)', re.S)
    blob = d[LO:HI]
    for m in pat.finditer(blob):
        a = RVA + m.start()
        vt = a + 7 + struct.unpack('<i', m.group(2))[0]
        st_at = RVA + m.start(3) - 2
        inst = st_at + 7 + struct.unpack('<i', m.group(4))[0]
        if not (BEGIN <= inst < END): continue
        name = rtti_name(vt)
        if not name: continue
        op = struct.unpack('<H', rd(inst + 0x0C, 2))[0]
        out.setdefault(name, op)
    return out

if __name__ == '__main__':
    t = table()
    old = {r['name']: int(r['id']) for r in csv.DictReader(open(
        'C:/Users/asher/CrimsonDesert-SeamlessCoop/upstream/cdmp-native/docs/research/data/troc_messages_2976.csv'))}
    with open('C:/Users/asher/CrimsonDesert-SeamlessCoop/build/native-port/troc_3549.csv', 'w', newline='') as f:
        w = csv.writer(f); w.writerow(['name', 'id_3549', 'id_2976'])
        for n in sorted(t): w.writerow([n, t[n], old.get(n, '')])
    both = [n for n in t if n in old]
    print('3549 names', len(t), '| 2976 names', len(old), '| in both', len(both),
          '| same id', sum(1 for n in both if t[n] == old[n]), '| ids unique', len(set(t.values())) == len(t))
    for n in ('RemoveActorAck', 'MoveActorReq', 'MoveActorAck', 'StartActionReq', 'AddItemToInventoryAck', 'InteractionDoStepReq'):
        print(n, old.get(n), '->', t.get(n))
