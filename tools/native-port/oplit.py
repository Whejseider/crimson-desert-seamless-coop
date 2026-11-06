# Bare opcode literals of the native source (2976 numbers). A literal is rewritten to its 3549 number only when the
# same line names that very message (e.g. `{2147, ...},  // VaryHPAck`); every other 4-digit number that is a 2976
# opcode and sits in an opcode-looking line is listed for review (build/native-port/oplit_review.txt).
#   python oplit.py [--apply]
import csv, re, sys, glob, os
ROOT = 'C:/Users/asher/CrimsonDesert-SeamlessCoop/upstream/cdmp-native/' + os.environ.get('NATIVE_DIR', 'src') + '/'
rows = list(csv.DictReader(open('C:/Users/asher/CrimsonDesert-SeamlessCoop/build/native-port/troc_3549.csv')))
new_by_name = {r['name']: int(r['id_3549']) for r in rows}
old = {r['name']: int(r['id']) for r in csv.DictReader(open(ROOT + '../docs/research/data/troc_messages_2976.csv'))}
name_by_old = {v: k for k, v in old.items()}
NUM = re.compile(r'(?<![\w.])([23]\d{3})(?![\w.])')
OPCTX = re.compile(r'\b(op|opcode|Ack|Req|Nak|Noti|troc|packet|case)\b|Ack\b|Req\b|Nak\b', re.I)
apply = '--apply' in sys.argv
done, review = 0, []
for f in sorted(glob.glob(ROOT + '**/*.h', recursive=True) + glob.glob(ROOT + '**/*.cpp', recursive=True)):
    lines = open(f, encoding='utf-8', newline='').read().split('\n')
    changed = False
    for i, line in enumerate(lines):
        code = line.split('//')[0]
        if re.search(r'constexpr\s+(?:std::)?uint(?:16|32)_t\s+(?:kOp|k\w*Op)', code): continue  # opmap.py's
        hits = [m for m in NUM.finditer(code) if int(m.group(1)) in name_by_old]
        if not hits: continue
        def ok(m):
            msg = name_by_old[int(m.group(1))]
            return msg in new_by_name and re.search(r'\b' + re.escape(msg) + r'\b', line)
        if all(ok(m) for m in hits):
            new = code
            for m in reversed(hits):
                new = new[:m.start(1)] + str(new_by_name[name_by_old[int(m.group(1))]]) + new[m.end(1):]
            lines[i] = new + line[len(code):]; changed = True; done += len(hits)
        elif OPCTX.search(line):
            review.append('%s:%d: %s' % (os.path.relpath(f, ROOT), i + 1, line.strip()[:200]))
    if apply and changed: open(f, 'w', encoding='utf-8', newline='').write('\n'.join(lines))
open('C:/Users/asher/CrimsonDesert-SeamlessCoop/build/native-port/oplit_review.txt', 'w').write('\n'.join(review))
print('literals rewritten (named on their line):', done, '| lines to review:', len(review))
