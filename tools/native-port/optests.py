# Tests only (one-shot, after opmap/oplit/oplit2/opmulti with NATIVE_DIR=tests): every 2976 opcode literal in CODE
# (outside strings and comments) on lines still as committed, except lines about time or sizes, becomes its 3549
# number. Prints every change.
#   python optests.py [--apply]
import csv, re, sys, glob, os, subprocess
ROOT = 'C:/Users/asher/CrimsonDesert-SeamlessCoop/upstream/cdmp-native/tests/'
rows = list(csv.DictReader(open('C:/Users/asher/CrimsonDesert-SeamlessCoop/build/native-port/troc_3549.csv')))
new_by_name = {r['name']: int(r['id_3549']) for r in rows}
old = {r['name']: int(r['id']) for r in csv.DictReader(open(ROOT + '../docs/research/data/troc_messages_2976.csv'))}
name_by_old = {v: k for k, v in old.items() if k in new_by_name}
NUM = re.compile(r'(?<![\w.])([23]\d{3})(?![\w.])')
STR = re.compile(r'"(?:\\.|[^"\\])*"')
NOT_OPCODE = re.compile(r'_ms\b|Ms\b|\bms\b|ms\)|timeout|sleep|wait|size|len\b|bytes|_us\b|kb\b|radius|meters|\bm\b', re.I)
apply = '--apply' in sys.argv
n = 0
for f in sorted(glob.glob(ROOT + '**/*.cpp', recursive=True) + glob.glob(ROOT + '**/*.h', recursive=True)):
    rel = os.path.relpath(f, ROOT + '..').replace(os.sep, '/')
    head = subprocess.run(['git', '-C', ROOT + '..', 'show', 'HEAD:' + rel], capture_output=True).stdout
    head = head.decode('utf-8', 'replace').split('\n')
    lines = open(f, encoding='utf-8', newline='').read().split('\n')
    changed = False
    for i, line in enumerate(lines):
        if i >= len(head) or head[i].rstrip('\r') != line.rstrip('\r'): continue
        code = line.split('//')[0]
        if NOT_OPCODE.search(code): continue
        masked = STR.sub(lambda m: ' ' * len(m.group(0)), code)
        hits = [m for m in NUM.finditer(masked) if int(m.group(1)) in name_by_old]
        if not hits: continue
        new = code
        for m in reversed(hits):
            new = new[:m.start(1)] + str(new_by_name[name_by_old[int(m.group(1))]]) + new[m.end(1):]
        print('%s:%d: %s\n    -> %s' % (rel, i + 1, code.strip()[:140], new.strip()[:140]))
        lines[i] = new + line[len(code):]; changed = True; n += len(hits)
    if apply and changed: open(f, 'w', encoding='utf-8', newline='').write('\n'.join(lines))
print('literals:', n)
