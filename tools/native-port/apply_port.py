# Write ported addresses into upstream/cdmp-native sources (build 2976 -> 3549).
#
#   python tools/native-port/apply_port.py <file relative to src/mod> <port json> [--dry]
#
# For every `constexpr uintptr_t kName = 0x...` in the file whose name has a new value in the port json, the value is
# replaced; every byte array of that constant (kName + Head / Bytes / Aob / Sig ... in the same file) is re-read from
# the 3549 exe at the new address, with the same length, so the module's own byte checks pass on 3549.
# Only use it for addresses verified by other means (vtable slots, callers, neighbours: docs/research/native-port-3549.json),
# and diff each re-read array against the old one: only embedded displacements may differ.
import json, os, re, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from pe49 import rd

SRC = 'C:/Users/asher/CrimsonDesert-SeamlessCoop/upstream/cdmp-native/src/mod/'
rel, port_path = sys.argv[1], sys.argv[2]
dry = '--dry' in sys.argv
port = json.load(open(port_path))
new = {k: int(v[1], 16) for k, v in port.items() if isinstance(v, list) and v[1]}
s = open(SRC + rel, encoding='utf-8', newline='').read()
changed = []
SUFFIXES = ('Head', 'Bytes', 'Aob', 'Sig', 'HeadBytes', 'PrologueBytes')
COMMENT = re.compile(r'//[^\r\n]*')
HEXBYTE = re.compile(r'0x[0-9A-Fa-f]{1,2}\b')


def fmt_bytes(b, indent):
    rows = [', '.join('0x%02X' % x for x in b[i:i + 12]) for i in range(0, len(b), 12)]
    return (',\n' + ' ' * indent).join(rows)


for name, addr in new.items():
    m = re.search(r'(constexpr\s+uintptr_t\s+' + name + r'\s*=\s*)(0x[0-9A-Fa-f]+)', s)
    if not m:
        continue
    old = int(m.group(2), 16)
    s = s[:m.start(2)] + '0x%X' % addr + s[m.end(2):]
    changed.append('%s 0x%X -> 0x%X' % (name, old, addr))
    for suf in SUFFIXES:
        # one search per suffix on the current text (each replacement shifts what follows)
        am = re.search(r'(constexpr\s+(?:std::)?uint8_t\s+' + name + suf + r'\[\]\s*=\s*\{)([^}]*)(\})', s)
        if not am:
            continue
        n = len(HEXBYTE.findall(COMMENT.sub('', am.group(2))))  # hex in comments is not array content
        fresh = rd(addr, n)
        if len(fresh) != n:
            print('cannot read %d bytes at 0x%X for %s%s' % (n, addr, name, suf))
            continue
        indent = am.end(1) - (s.rfind('\n', 0, am.start()) + 1)
        s = s[:am.start(2)] + fmt_bytes(fresh, indent) + s[am.end(2):]
        changed.append('  %s%s: %d bytes re-read' % (name, suf, n))
print('\n'.join(changed) or 'nothing to change')
if not dry:
    open(SRC + rel, 'w', encoding='utf-8', newline='').write(s)
