# Native port 3549 -> 3576, step 3b: byte lists written inline after an address, `{0xADDR, {0x.., ...}` (struct
# initialisers heads3576.py does not see). ADDR is already the 3576 value; the list must equal the 3549 bytes at the
# old address; then it is replaced by the 3576 bytes at ADDR.
import glob, json, re, sys
sys.path.insert(0, 'C:/Users/asher/CrimsonDesert-SeamlessCoop/build/port3576')
import portmap as p
from heads3576 import mapping
from scan3576 import REPO
inv = {n: o for o, n in mapping().items()}
PAT = re.compile(r'\{\s*0x([0-9A-Fa-f]{5,8})\s*,\s*\{((?:\s*0x[0-9A-Fa-f]{2}\s*,?)+)\}')
dry = '--dry' in sys.argv
for path in glob.glob(REPO + '/src/**/*.*', recursive=True):
    if not path.endswith(('.cpp', '.h')):
        continue
    raw = open(path, encoding='utf-8', newline='').read()
    out, n = [], 0
    for m in PAT.finditer(raw):
        new = int(m.group(1), 16)
        if new not in inv:
            continue
        vals = re.findall(r'0x([0-9A-Fa-f]{2})', m.group(2))
        b = bytes(int(x, 16) for x in vals)
        nb = p.NEW.rd(new, len(b))
        if nb == b:
            continue
        ob = p.OLD.rd(inv[new], len(b))
        if ob != b:
            print('?? %s 0x%X: listed bytes are not the 3549 bytes either' % (path[len(REPO) + 1:], new))
            continue
        txt = ', '.join('0x%02X' % x for x in nb)
        out.append((m.start(2), m.end(2), txt))
        print('%s 0x%X: %s -> %s' % (path[len(REPO) + 1:], new, b.hex(' '), nb.hex(' ')))
    if out and not dry:
        for s, e, t in reversed(out):
            raw = raw[:s] + t + raw[e:]
        open(path, 'w', encoding='utf-8', newline='').write(raw)
