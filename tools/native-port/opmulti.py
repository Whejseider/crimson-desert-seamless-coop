# Third pass (one-shot): declarations with several opcode constants on one line, `constexpr uint16_t kA = n, kB = m`.
# opmap.py mapped only the first constant of such a line, and oplit2.py skips lines already touched, so the others are
# still 2976 numbers: map every `kX = n` whose number is unchanged since HEAD (2976) on such a touched line.
#   python opmulti.py [--apply]
import csv, re, sys, glob, os, subprocess
ROOT = 'C:/Users/asher/CrimsonDesert-SeamlessCoop/upstream/cdmp-native/' + os.environ.get('NATIVE_DIR', 'src') + '/'
rows = list(csv.DictReader(open('C:/Users/asher/CrimsonDesert-SeamlessCoop/build/native-port/troc_3549.csv')))
new_by_name = {r['name']: int(r['id_3549']) for r in rows}
old = {r['name']: int(r['id']) for r in csv.DictReader(open(ROOT + '../docs/research/data/troc_messages_2976.csv'))}
name_by_old = {v: k for k, v in old.items()}
ASSIGN = re.compile(r'\b(k\w+)\s*=\s*([23]\d{3})\b')
apply = '--apply' in sys.argv
n = 0
for f in sorted(glob.glob(ROOT + '**/*.h', recursive=True) + glob.glob(ROOT + '**/*.cpp', recursive=True)):
    rel = os.path.relpath(f, ROOT + '..').replace(os.sep, '/')
    head = subprocess.run(['git', '-C', ROOT + '..', 'show', 'HEAD:' + rel], capture_output=True).stdout
    head = head.decode('utf-8', 'replace').split('\n')
    lines = open(f, encoding='utf-8', newline='').read().split('\n')
    changed = False
    for i, line in enumerate(lines):
        if i >= len(head) or head[i].rstrip('\r') == line.rstrip('\r'): continue  # untouched lines: oplit2's
        code = line.split('//')[0]
        if 'constexpr' not in code and not re.match(r'\s*k\w+\s*=', code): continue
        before = {m.group(1): int(m.group(2)) for m in ASSIGN.finditer(head[i].split('//')[0])}
        def rep(m):
            global n
            num = int(m.group(2))
            if before.get(m.group(1)) != num or num not in name_by_old: return m.group(0)  # mapped already / not one
            msg = name_by_old[num]
            if msg not in new_by_name: return m.group(0)
            n += 1
            print('%s:%d: %s = %d -> %d [%s]' % (rel, i + 1, m.group(1), num, new_by_name[msg], msg))
            return '%s = %d' % (m.group(1), new_by_name[msg])
        new = ASSIGN.sub(rep, code)
        if new != code:
            lines[i] = new + line[len(code):]; changed = True
    if apply and changed: open(f, 'w', encoding='utf-8', newline='').write('\n'.join(lines))
print('constants:', n)
