# Resolve the old base's per-build offset tables (using build_XXXX::k chains) and pair them with the native repo's
# 2976 values by name. Prints the (2976 -> 3549) pairs as JSON.
import re, json, sys
G = 'cd-coop-3446/src/mod/game/'
def parse(p):
    s = open(p, encoding='utf-8', errors='replace').read()
    vals = {m.group(1): int(m.group(2), 16) for m in re.finditer(r'constexpr\s+(?:std::)?u?int(?:ptr|64|32|16)?_t\s+(k\w+)\s*=\s*(0x[0-9A-Fa-f]+)', s)}
    uses = {m.group(2): m.group(1) for m in re.finditer(r'using\s+build_(\d+)::(k\w+)\s*;', s)}
    return vals, uses
builds = {}
for b in ['3424', '3446', '3460', '3479', '3527', '3549']:
    try: builds[b] = parse(G + f'offsets_{b}.h')
    except FileNotFoundError: pass
def val(b, k, depth=0):
    vals, uses = builds[b]
    if k in vals: return vals[k]
    if k in uses and depth < 10: return val(uses[k], k, depth + 1)
    return None
old, _ = parse('cdmp-native/src/mod/game/offsets.h')
names = set(old) | set(builds['3549'][0]) | set(builds['3549'][1])
pairs = {}
for k in sorted(names):
    a, b = old.get(k), val('3549', k)
    if a and b and a > 0x10000 and b > 0x10000: pairs[k] = [a, b]
json.dump(pairs, open('../build/native-port/pairs_offsets.json', 'w'), indent=1)
print(len(old), 'native names;', len(pairs), 'pairs')
for k, (a, b) in sorted(pairs.items(), key=lambda x: x[1][0]): print('%-36s %8X -> %8X  %+8X' % (k, a, b, b - a))
