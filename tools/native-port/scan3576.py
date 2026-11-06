# Native port 1.0.0.3549 -> 1.0.0.3576, step 1: every game address the 3549 port wrote into upstream/cdmp-native
# (non-comment hex literals on lines added since ByJunter's base commit), mapped with build/port3576/portmap.py:
# code by its masked instructions, data / vtables through the code that addresses them, vtables cross-checked
# through RTTI by class name. Writes build/native-port/map_3576.json; prints what needs a look.
#   python scan3576.py
import json, os, re, struct, subprocess, sys
sys.path.insert(0, 'C:/Users/asher/CrimsonDesert-SeamlessCoop/build/port3576')
import portmap as p

REPO = 'C:/Users/asher/CrimsonDesert-SeamlessCoop/upstream/cdmp-native'
BASE_COMMIT = '3cbb03c'  # ByJunter's NativeServer, before the Co-op+ port
OUT = 'C:/Users/asher/CrimsonDesert-SeamlessCoop/build/native-port/map_3576.json'
IB = 0x140000000
CODE_HI = 0x5595000            # 3549: .xtext ends here (code), then .debug .. data
DATA_LO, DATA_HI = 0x5595000, 0x7A41000


def added_lines():
    diff = subprocess.run(['git', '-C', REPO, 'diff', BASE_COMMIT, 'HEAD', '-U0', '--', 'src'],
                          capture_output=True, text=True, encoding='utf-8', errors='replace').stdout
    out, f, ln = [], None, 0
    for line in diff.splitlines():
        if line.startswith('+++ '):
            f = line[6:] if line.startswith('+++ b/') else None
        elif line.startswith('@@'):
            m = re.search(r'\+(\d+)', line)
            ln = int(m.group(1)) if m else 0
        elif line.startswith('+') and f:
            out.append((f, ln, line[1:]))
            ln += 1
    return out


def code_part(s):
    i = s.find('//')
    return s if i < 0 else s[:i]


def rtti_name(img, vt):
    try:
        col = struct.unpack('<Q', img.rd(vt - 8, 8))[0] - IB
        if not (0 < col < 0x14000000) or struct.unpack('<I', img.rd(col, 4))[0] != 1:
            return None
        td = struct.unpack('<I', img.rd(col + 0x0C, 4))[0]
        return img.rd(td + 0x10, 200).split(b'\0')[0].decode('latin1')
    except Exception:
        return None


def vt_by_name(img, name):
    import rtti_vt
    rtti_vt.IMG = img
    return rtti_vt.vtables(name)


def main():
    lits = {}
    for f, ln, s in added_lines():
        for m in re.finditer(r'\b0x([0-9A-Fa-f]{5,8})\b', code_part(s)):
            v = int(m.group(1), 16)
            if 0x1000 <= v < DATA_HI:
                lits.setdefault(v, []).append('%s:%d' % (f, ln))
    print(len(lits), 'candidate address literals')
    res = {}
    for v in sorted(lits):
        e = {'at': lits[v][:6]}
        if v < CODE_HI:
            new, hits, second = p.code(v)
            e.update(kind='code', new=new, hits=hits, second=second,
                     same16=bool(new) and p.OLD.rd(v, 16) == p.NEW.rd(new, 16))
        else:
            nm = rtti_name(p.OLD, v)
            if nm:
                vts = vt_by_name(p.NEW, nm)
                e.update(kind='vtable', name=nm, new=vts[0] if len(vts) == 1 else None, cands=vts)
            else:
                new, votes, sites = p.data(v)
                e.update(kind='data', new=new, votes=votes, sites=sites)
        res['0x%X' % v] = e
    os.makedirs(os.path.dirname(OUT), exist_ok=True)
    json.dump(res, open(OUT, 'w'), indent=1)
    look = 0
    for k, e in res.items():
        bad = (e['new'] is None or (e['kind'] == 'code' and (e['hits'] != 1 or not e['same16'])) or
               (e['kind'] == 'data' and e['votes'] < max(1, e['sites'] // 2 + 1)))
        if bad:
            look += 1
            print('LOOK %-10s %-6s -> %s %s  %s' % (k, e['kind'], '0x%X' % e['new'] if e['new'] else None,
                  {x: e[x] for x in ('hits', 'same16', 'votes', 'sites', 'name', 'cands') if x in e}, e['at'][0]))
    print(len(res), 'mapped,', look, 'to look at; ->', OUT)


if __name__ == '__main__':
    main()
