# Native port 3549 -> 3576, step 1c: drop literals that were already in ByJunter's base version of the same file
# (2976 values on lines the port touched for another reason, e.g. owner_fanout_writers.h's opcodes).
import json, re, subprocess
from scan3576 import REPO, BASE_COMMIT, OUT
m = json.load(open(OUT))
v = json.load(open(OUT.replace('.json', '_verdict.json')))
base_cache = {}
def base_lits(f):
    if f not in base_cache:
        r = subprocess.run(['git', '-C', REPO, 'show', '%s:%s' % (BASE_COMMIT, f)], capture_output=True, text=True,
                           encoding='utf-8', errors='replace')
        base_cache[f] = {int(x, 16) for x in re.findall(r'\b0x([0-9A-Fa-f]{5,8})\b', r.stdout)} if r.returncode == 0 else set()
    return base_cache[f]
keep, drop = {}, {}
for k, e in m.items():
    val = int(k, 16)
    files = {a.rsplit(':', 1)[0] for a in e['at']}
    (drop if all(val in base_lits(f) for f in files) else keep)[k] = v[k]
from collections import Counter
print('kept', len(keep), Counter(x[0] for x in keep.values()))
print('dropped (base values)', len(drop), Counter(x[0] for x in drop.values()))
json.dump(sorted(drop), open(OUT.replace('.json', '_base.json'), 'w'))
for k, (st, nv) in sorted(keep.items(), key=lambda t: int(t[0], 16)):
    if st not in ('ok', 'multi_ok'):
        print(st, k, nv, m[k]['at'][0])
