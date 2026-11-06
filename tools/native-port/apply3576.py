# Native port 1.0.0.3549 -> 1.0.0.3576, step 2: rewrite the addresses scan3576.py mapped (build/native-port/
# map_3576.json) in upstream/cdmp-native. Only the code part of lines the 3549 port added is touched (comments and
# ByJunter's unported 2976 values stay as they are). OVERRIDES holds the values decided by hand (see PROGRESS.md);
# SKIP the ones left alone. Prints every file changed and any literal left unmapped.
#   python apply3576.py [--dry]
import json, re, sys
from scan3576 import REPO, added_lines, code_part, OUT

OVERRIDES = {    # old -> new, decided by hand (judge3576 / build/port3576/stubs.py; PROGRESS 2026-11-06)
    0x23FD080: 0x23FDDC0,   # CommonVehicleActorComponent vf53 (RTTI slot 53; its caller agrees)
    0xB4D8D0: 0xB4DCF0,     # ClientVehicleActorComponent vf53 stub (RTTI slot 53)
    0xB4D8F0: 0xB4DD10,     # ClientVehicleActorComponent vf54 (RTTI slot 54)
    0x23FDCC0: 0x23FEA20,   # RideOff thunk (4 callers agree)
    0x19B2080: 0x19B2CC0,   # save manager refresh-slots stub (4 callers agree)
    0x22A2880: 0x22A34C0,   # owner-user stub (4 callers agree)
    0x2548E40: 0x2549C10,   # GetOrCreateSlot stub (3 callers agree)
    0x2BAFA40: 0x2BB06B0,   # ServerStatusActorComponent vf10 stub (RTTI slot 10)
    0x19318C0: 0x1932500,   # is-local function (4 callers agree)
    0x193190C: 0x193254C,   # its jump table (function + 0x4C)
    0x976634: 0x9769A4,     # the mode function's jump table (function 0x976560 -> 0x9768D0, +0x370)
    0x2D977D0: 0x2D98440,   # two identical functions, both +0xC70 like their neighbours
    0x2D97840: 0x2D984B0,
    0x6EC2378: 0x6EC4378,   # kTrocInstancesEnd (= begin's +0x2000; an end is never referenced)
}
SKIP = {0x100000, 0x6C00000, 0x7400000}  # a flag bit; server.cpp's scan range (wide enough on 3576)


def main(dry):
    m = json.load(open(OUT))
    verdict = json.load(open(OUT.replace('.json', '_verdict.json')))
    base = {int(k, 16) for k in json.load(open(OUT.replace('.json', '_base.json')))}  # ByJunter's 2976 values
    for v in base:
        m.pop('0x%X' % v, None)
    new_of = {}
    for k in m:
        v = int(k, 16)
        if v in SKIP:
            continue
        st, nv = verdict[k]
        if v in OVERRIDES:
            new_of[v] = OVERRIDES[v]
        elif st in ('ok', 'multi_ok'):
            new_of[v] = nv
    want = {}
    for f, ln, _s in added_lines():
        want.setdefault(f, set()).add(ln)
    missing, changed = set(), {}
    for f, lines in want.items():
        path = REPO + '/' + f
        try:
            raw = open(path, encoding='utf-8', newline='').read()
        except FileNotFoundError:
            continue
        rows = raw.split('\n')
        n = 0
        for i in sorted(lines):
            if i - 1 >= len(rows):
                continue
            row = rows[i - 1]
            cut = row.find('//')
            code, rest = (row, '') if cut < 0 else (row[:cut], row[cut:])

            def sub(mm):
                nonlocal n
                v = int(mm.group(1), 16)
                if '0x%X' % v not in m:
                    return mm.group(0)
                if v not in new_of:
                    missing.add(v)
                    return mm.group(0)
                if new_of[v] == v:
                    return mm.group(0)
                n += 1
                width = len(mm.group(1))
                return '0x%0*X' % (width, new_of[v])
            code2 = re.sub(r'\b0x([0-9A-Fa-f]{5,8})\b', sub, code)
            rows[i - 1] = code2 + rest
        if n:
            changed[f] = n
            if not dry:
                open(path, 'w', encoding='utf-8', newline='').write('\n'.join(rows))
    for f, n in sorted(changed.items()):
        print('%4d  %s' % (n, f))
    print(sum(changed.values()), 'literals in', len(changed), 'files', '(dry run)' if dry else '')
    if missing:
        print('LEFT AS 3549 (no mapping):', ' '.join('0x%X' % v for v in sorted(missing)))


if __name__ == '__main__':
    main('--dry' in sys.argv)
