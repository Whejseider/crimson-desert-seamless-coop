# Native port 3549 -> 3576, step 1b: judge scan3576's results against the regional shift of unique neighbours.
import bisect, json, sys
sys.path.insert(0, 'C:/Users/asher/CrimsonDesert-SeamlessCoop/build/port3576')
import portmap as p
from scan3576 import OUT
m = json.load(open(OUT))
anch = sorted((int(k, 16), e['new'] - int(k, 16)) for k, e in m.items() if e['kind'] == 'code' and e['hits'] == 1 and e['new'])
xs = [a for a, _ in anch]
def near_shift(v):
    i = bisect.bisect_left(xs, v)
    c = [anch[j] for j in (i - 1, i) if 0 <= j < len(anch)]
    a, s = min(c, key=lambda t: abs(t[0] - v))
    return s, abs(a - v)
verdict = {}
for k, e in m.items():
    v = int(k, 16)
    if e['kind'] == 'code':
        if e['new'] is None:
            verdict[k] = ('nomap', None); continue
        s, d = near_shift(v)
        sh = e['new'] - v
        if e['hits'] == 1:
            ok = abs(sh - s) <= 0x200 or d > 0x20000
            verdict[k] = ('ok' if ok else 'odd1', e['new'])
        else:
            # several hits: take the one whose shift equals the neighbours' shift
            rx, _ = p.pattern(p.OLD, v, 40)
            hits = p.find(p.NEW, rx) if rx else []
            best = [h for h in hits if abs((h - v) - s) <= 0x100]
            verdict[k] = ('multi_ok', best[0]) if len(best) == 1 else ('multi?', [hex(h) for h in best][:4])
    elif e['kind'] == 'vtable':
        verdict[k] = ('ok' if e['new'] else 'vt?', e['new'])
    else:
        ok = e['new'] and e['votes'] >= max(1, (e['sites'] + 1) // 2)
        verdict[k] = ('ok' if ok else 'data?', e['new'])
from collections import Counter
print(Counter(v[0] for v in verdict.values()))
json.dump(verdict, open(OUT.replace('.json', '_verdict.json'), 'w'), indent=0)
for k, (st, nv) in sorted(verdict.items(), key=lambda t: int(t[0], 16)):
    if st not in ('ok', 'multi_ok'):
        print(st, k, nv, m[k]['at'][0], m[k].get('sites', ''), m[k].get('votes', ''))
