#!/usr/bin/env python3
"""Check every opcode number the mod hardcodes against a game build's live Troc table.

After a game update the opcodes move. The kOp* constants in offsets_<build>.h were regenerated on
3460, but the relay table (kRelayedAcks) and two inline `op = 2815` sites kept their 3424 numbers
until 2026-11-02, silently relaying the wrong messages. This finds such leftovers.

usage: python check_opcodes.py TROC_TABLE.txt [SRC_DIR]
  TROC_TABLE: "Name opcode kind" per line (build/port3479/troc_3479.txt, made by troc.py live)
  SRC_DIR:    defaults to upstream/cd-coop-3446/src/mod
Checked: `kOpName = N` (the name must be the message at N, or end it, e.g. AttackTargetAck for
PostProcessAttackResult_AttackTargetAck), `op = N ... // Name`, and `{N, ...}, // Name` rows of
kRelayedAcks in the newest offsets header. Older offsets_*.h files are skipped.
Exit code 1 if anything does not match.
"""
import glob
import os
import re
import sys

troc_path = sys.argv[1]
src = sys.argv[2] if len(sys.argv) > 2 else os.path.join(os.path.dirname(__file__), '..', '..', 'upstream',
                                                        'cd-coop-3446', 'src', 'mod')
by_op, by_name = {}, {}
for line in open(troc_path, encoding='utf-8', errors='replace'):
    p = line.split()
    if len(p) >= 2 and p[1].isdigit():
        by_op.setdefault(int(p[1]), []).append(p[0])
        by_name[p[0]] = int(p[1])

headers = sorted(glob.glob(os.path.join(src, 'game', 'offsets_*.h')))
newest = headers[-1] if headers else None
files = [f for f in glob.glob(os.path.join(src, '**', '*.cpp'), recursive=True) +
         glob.glob(os.path.join(src, '**', '*.h'), recursive=True)
         if 'offsets_' not in os.path.basename(f)]
# The headers that still define their own opcodes for the current build (3460 holds them for 3479).
files += [h for h in headers if h >= os.path.join(src, 'game', 'offsets_3460.h')]

const = re.compile(r'\b(kOp\w+)\s*=\s*(\d{4})\b')
inline = re.compile(r'\bop\s*=\s*(\d{4})\b.*//\s*([A-Z]\w+(?:Req|Ack|Nak))')
row = re.compile(r'^\s*\{(\d{4}),.*//\s*([A-Z]\w+(?:Req|Ack|Nak))')


def ok(n, want):
    return any(name == want or name.endswith('_' + want) for name in by_op.get(n, []))


bad = 0
for f in files:
    for i, line in enumerate(open(f, encoding='utf-8', errors='replace'), 1):
        hits = [(int(m.group(2)), m.group(1)[3:]) for m in const.finditer(line)]
        if m := inline.search(line):
            hits.append((int(m.group(1)), m.group(2)))
        if m := row.search(line):
            hits.append((int(m.group(1)), m.group(2)))
        # Opcode tables must not come from an older build's header (that is how kRelayedAcks kept 3424).
        if 'offsets_' in os.path.basename(f) and re.search(r'using build_\d+::(kOp\w+|kRelayedAcks)\b', line) \
                and not re.search(r'using build_3460::', line):
            bad += 1
            print(f'{os.path.relpath(f, src)}:{i}: inherited from an older build, re-check: {line.strip()}')
        for n, want in hits:
            if want not in by_name and not any(k.endswith('_' + want) for k in by_name):
                continue  # not a message name (a local alias); nothing to compare with
            if not ok(n, want):
                bad += 1
                print(f'{os.path.relpath(f, src)}:{i}: {want} = {n}, but {n} is {by_op.get(n, ["nothing"])}; '
                      f'{want} is {by_name.get(want)}')
print(f'{bad} mismatch(es)' if bad else 'all hardcoded opcodes match the table')
sys.exit(1 if bad else 0)
