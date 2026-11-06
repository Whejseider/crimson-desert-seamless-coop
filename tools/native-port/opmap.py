# Opcode constants of the native source: 2976 number -> 3549 number by message name (troc49.py's table).
#   python opmap.py           report: every `kOp<Name> = <n>` / `= <n>;  // <Name>` constant, its name check, the new id
#   python opmap.py --apply   rewrite the numbers (only constants whose 2976 number is a known message)
import csv, re, sys, glob, os
ROOT = 'C:/Users/asher/CrimsonDesert-SeamlessCoop/upstream/cdmp-native/' + os.environ.get('NATIVE_DIR', 'src') + '/'
rows = list(csv.DictReader(open('C:/Users/asher/CrimsonDesert-SeamlessCoop/build/native-port/troc_3549.csv')))
new_by_name = {r['name']: int(r['id_3549']) for r in rows}
old = {r['name']: int(r['id']) for r in csv.DictReader(open(ROOT + '../docs/research/data/troc_messages_2976.csv'))}
name_by_old = {v: k for k, v in old.items()}
CONST = re.compile(r'(constexpr\s+(?:std::)?uint(?:16|32)_t\s+(kOp\w*|k\w*Op\w*|kOpcode\w*)\s*=\s*)(\d{4})\b')
apply = '--apply' in sys.argv
bad, n = [], 0
for f in sorted(glob.glob(ROOT + '**/*.h', recursive=True) + glob.glob(ROOT + '**/*.cpp', recursive=True)):
    s = open(f, encoding='utf-8', newline='').read()
    def rep(m):
        global n
        num = int(m.group(3)); msg = name_by_old.get(num)
        if msg is None or msg not in new_by_name:
            bad.append('%s: %s = %d (%s)' % (os.path.relpath(f, ROOT), m.group(2), num, msg or 'not a 2976 message'))
            return m.group(0)
        n += 1
        return m.group(1) + str(new_by_name[msg])
    t = CONST.sub(rep, s)
    if apply and t != s: open(f, 'w', encoding='utf-8', newline='').write(t)
print('constants mapped:', n); print('left as they are:', len(bad)); print('\n'.join(bad[:40]))
