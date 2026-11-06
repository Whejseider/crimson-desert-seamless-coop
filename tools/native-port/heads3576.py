# Native port 1.0.0.3549 -> 1.0.0.3576, step 3: the byte arrays the modules compare against the game (heads,
# stubs, jump tables). Each array on a line the 3549 port added is found in the 3549 exe at (or within 0x400 of) an
# address of the same file that apply3576.py mapped; the same span is read at the mapped 3576 address and written
# back. Arrays found nowhere are listed (left as they are).
#   python heads3576.py [--dry]
import json, re, sys
sys.path.insert(0, 'C:/Users/asher/CrimsonDesert-SeamlessCoop/build/port3576')
import portmap as p
from scan3576 import REPO, OUT, added_lines
from apply3576 import OVERRIDES, SKIP

ARR = re.compile(r'(\w+)\[\]\s*=\s*\{([^{}]*)\}', re.S)


def mapping():
    m = json.load(open(OUT))
    verdict = json.load(open(OUT.replace('.json', '_verdict.json')))
    base = set(json.load(open(OUT.replace('.json', '_base.json'))))
    out = {}
    for k in m:
        if k in base or int(k, 16) in SKIP:
            continue
        v = int(k, 16)
        st, nv = verdict[k]
        if v in OVERRIDES:
            out[v] = OVERRIDES[v]
        elif st in ('ok', 'multi_ok'):
            out[v] = nv
    return out


def main(dry):
    mp = mapping()
    by_file = {}
    for k, e in json.load(open(OUT)).items():
        v = int(k, 16)
        if v in mp:
            for a in e['at']:
                by_file.setdefault(a.rsplit(':', 1)[0], set()).add(v)
    touched = {}
    for f, ln, _s in added_lines():
        touched.setdefault(f, set()).add(ln)
    changed, unfound, same = [], [], 0
    for f, lines in touched.items():
        path = REPO + '/' + f
        try:
            text = open(path, encoding='utf-8', newline='').read()
        except FileNotFoundError:
            continue
        addrs = sorted(by_file.get(f, ()))
        out, pos = [], 0
        for mm in ARR.finditer(text):
            line = text.count('\n', 0, mm.start()) + 1
            body = mm.group(2)
            vals = re.findall(r'0x([0-9A-Fa-f]{2})\b', body)
            span = set(range(line, text.count('\n', 0, mm.end()) + 2))
            if len(vals) < 4 or not (span & lines) or re.search(r'0x[0-9A-Fa-f]{3,}', body):
                continue
            b = bytes(int(x, 16) for x in vals)
            hit = None
            for a in addrs:
                if p.OLD.rd(a, len(b)) == b:
                    hit = (a, 0)
                    break
            if hit is None:
                for a in addrs:
                    win = p.OLD.rd(a - 0x400, 0x800 + len(b))
                    i = win.find(b)
                    if i >= 0:
                        hit = (a, i - 0x400)
                        break
            if hit is None:
                unfound.append('%s:%d %s (%d bytes)' % (f, line, mm.group(1), len(b)))
                continue
            a, off = hit
            nb = p.NEW.rd(mp[a] + off, len(b))
            if nb == b:
                same += 1
                continue
            # keep the layout: same number of values per line as before
            rows = body.split('\n')
            counts = [len(re.findall(r'0x[0-9A-Fa-f]{2}\b', r)) for r in rows]
            it = iter(nb)
            new_rows = []
            for r, c in zip(rows, counts):
                if c == 0:
                    new_rows.append(r)
                    continue
                lead = re.match(r'\s*', r).group(0)
                tail = ',' if r.rstrip().endswith(',') else ''
                new_rows.append(lead + ', '.join('0x%02X' % next(it) for _ in range(c)) + tail)
            out.append((mm.start(2), mm.end(2), '\n'.join(new_rows)))
            changed.append('%s:%d %s @0x%X%+d: %s -> %s' % (f, line, mm.group(1), mp[a], off, b[:8].hex(' '), nb[:8].hex(' ')))
        if out and not dry:
            for s, e, t in reversed(out):
                text = text[:s] + t + text[e:]
            open(path, 'w', encoding='utf-8', newline='').write(text)
    print('unchanged heads:', same)
    print('re-read (%d):' % len(changed))
    for c in changed:
        print('  ', c)
    print('NOT FOUND near a mapped address (%d):' % len(unfound))
    for u in unfound:
        print('  ', u)


if __name__ == '__main__':
    main('--dry' in sys.argv)
