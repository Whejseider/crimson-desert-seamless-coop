# Second pass after oplit.py (ONE-SHOT, like opmap.py and oplit.py: a second run would map 3549 numbers again):
# 2976 opcode literals in CODE (outside strings and comments) on lines with opcode syntax, and `uint16_t k<Message>`
# constants named after their message. Prints every change for review.
#   python oplit2.py [--apply]
import csv, re, sys, glob, os, subprocess
ROOT = 'C:/Users/asher/CrimsonDesert-SeamlessCoop/upstream/cdmp-native/' + os.environ.get('NATIVE_DIR', 'src') + '/'
rows = list(csv.DictReader(open('C:/Users/asher/CrimsonDesert-SeamlessCoop/build/native-port/troc_3549.csv')))
new_by_name = {r['name']: int(r['id_3549']) for r in rows}
old = {r['name']: int(r['id']) for r in csv.DictReader(open(ROOT + '../docs/research/data/troc_messages_2976.csv'))}
name_by_old = {v: k for k, v in old.items()}
NUM = re.compile(r'(?<![\w.])([23]\d{3})(?![\w.])')
STR = re.compile(r'"(?:\\.|[^"\\])*"')
SYNTAX = re.compile(r'\bop(code)?\s*[!=]=|\bcase\s+[23]\d{3}\s*:|packet\(\s*[23]\d{3}|\bop\s*=\s*[23]\d{3}|'
                    r'uint16_t\s+k\w+\s*=\s*[23]\d{3}|\{\s*[23]\d{3}\s*,|kOp|opcode')
apply = '--apply' in sys.argv
n = 0
for f in sorted(glob.glob(ROOT + '**/*.h', recursive=True) + glob.glob(ROOT + '**/*.cpp', recursive=True)):
    lines = open(f, encoding='utf-8', newline='').read().split('\n')
    # only lines still as committed (2976 numbers): the earlier passes already wrote 3549 numbers elsewhere
    rel = os.path.relpath(f, ROOT + '..').replace(os.sep, '/')
    head = subprocess.run(['git', '-C', ROOT + '..', 'show', 'HEAD:' + rel], capture_output=True).stdout
    head = head.decode('utf-8', 'replace').split('\n')
    changed = False
    for i, line in enumerate(lines):
        if i >= len(head) or head[i].rstrip('\r') != line.rstrip('\r'): continue
        code = line.split('//')[0]
        masked = STR.sub(lambda m: ' ' * len(m.group(0)), code)
        if not SYNTAX.search(masked): continue
        hits = [m for m in NUM.finditer(masked) if int(m.group(1)) in name_by_old
                and name_by_old[int(m.group(1))] in new_by_name]
        if not hits: continue
        new = code
        for m in reversed(hits):
            new = new[:m.start(1)] + str(new_by_name[name_by_old[int(m.group(1))]]) + new[m.end(1):]
        print('%s:%d: %s\n    -> %s   [%s]' % (os.path.relpath(f, ROOT), i + 1, code.strip()[:150], new.strip()[:150],
                                          ', '.join(name_by_old[int(m.group(1))] for m in hits)))
        lines[i] = new + line[len(code):]; changed = True; n += len(hits)
    if apply and changed: open(f, 'w', encoding='utf-8', newline='').write('\n'.join(lines))
print('literals:', n)
