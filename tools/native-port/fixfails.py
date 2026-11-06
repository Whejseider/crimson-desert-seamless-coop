# Unit-test fixtures (2976 opcodes) -> 3549, driven by the failures themselves: runs every test exe, takes each
# "FAIL <file>:<line>: ..." line, and on THAT line only replaces each 2976 opcode literal (outside strings and comments)
# by its 3549 number. Lines about time or sizes are left alone. Prints every change; --apply writes them.
# Only lines still as committed (git HEAD) are touched: a line already mapped, and still failing for another reason,
# must not be mapped twice (a 3549 number is often also some 2976 opcode). --base REF compares against REF instead of
# HEAD: the commit before the first fixture pass (12fff5f in the native repo) once a pass is committed.
#   python fixfails.py [--base REF] [--apply]   (then rebuild and run again: a fixed line can reveal the next one)
import csv, glob, os, re, subprocess, sys

NATIVE = 'C:/Users/asher/CrimsonDesert-SeamlessCoop/upstream/cdmp-native/'
CSV = 'C:/Users/asher/CrimsonDesert-SeamlessCoop/docs/research/troc-messages-3549.csv'
rows = list(csv.DictReader(open(CSV)))
new_by_old = {int(r['id_2976']): int(r['id_3549']) for r in rows if r['id_2976'] and r['id_3549']}
NUM = re.compile(r'(?<![\w.])([23]\d{3})(?![\w.])')
STR = re.compile(r'"(?:\\.|[^"\\])*"')
NOT_OPCODE = re.compile(r'_ms\b|Ms\b|\bms\b|ms\)|timeout|sleep|wait|size|len\b|bytes|_us\b|radius|meters', re.I)
FAIL = re.compile(r'FAIL (.+?):(\d+):')
CR = '\r'
apply = '--apply' in sys.argv
BASE = sys.argv[sys.argv.index('--base') + 1] if '--base' in sys.argv else 'HEAD'

fails = {}
for exe in sorted(glob.glob(NATIVE + 'build/bin/RelWithDebInfo/cdmp_tests*.exe')):
    try:
        out = subprocess.run([exe], capture_output=True, text=True, timeout=120).stdout
    except subprocess.TimeoutExpired:
        print('timeout:', os.path.basename(exe)); continue
    for m in FAIL.finditer(out):
        fails.setdefault(os.path.normpath(m.group(1)), set()).add(int(m.group(2)))

n = 0
for path, lines_failed in sorted(fails.items()):
    text = open(path, encoding='utf-8', newline='').read().split('\n')
    rel = os.path.relpath(path, NATIVE).replace(os.sep, '/')
    head = subprocess.run(['git', '-C', NATIVE, 'show', BASE + ':' + rel], capture_output=True).stdout
    head = head.decode('utf-8', 'replace').split('\n')
    changed = False
    for ln in sorted(lines_failed):
        line = text[ln - 1]
        if ln > len(head) or head[ln - 1].rstrip(CR) != line.rstrip(CR): continue  # changed since HEAD
        code = line.split('//')[0]
        if NOT_OPCODE.search(code): continue
        masked = STR.sub(lambda m: ' ' * len(m.group(0)), code)
        hits = [m for m in NUM.finditer(masked) if int(m.group(1)) in new_by_old]
        if not hits: continue
        new = code
        for m in reversed(hits):
            new = new[:m.start(1)] + str(new_by_old[int(m.group(1))]) + new[m.end(1):]
        if new == code: continue
        print('%s:%d\n    %s\n -> %s' % (rel, ln, code.strip()[:150], new.strip()[:150]))
        text[ln - 1] = new + line[len(code):]
        changed = True; n += len(hits)
    if apply and changed: open(path, 'w', encoding='utf-8', newline='').write('\n'.join(text))
print('failing lines', sum(len(v) for v in fails.values()), 'in', len(fails), 'files | literals changed', n)
