# Inventory of the native repo's game addresses (build 2976) and a first port to 3549: each `constexpr uintptr_t kX`
# with a byte array kX<Head|Bytes|...> is searched (exact bytes) in the 3549 exe. Writes heads.json.
import json, re, struct, sys, glob, os
EXE = 'C:/Users/asher/CrimsonDesert-SeamlessCoop/build/exe-archive/CrimsonDesert-1.0.0.3549.exe'
SRC = 'C:/Users/asher/CrimsonDesert-SeamlessCoop/upstream/cdmp-native/src/mod'
d = open(EXE, 'rb').read()
pe = struct.unpack_from('<I', d, 0x3c)[0]
nsec = struct.unpack_from('<H', d, pe + 6)[0]; optsz = struct.unpack_from('<H', d, pe + 20)[0]
secs = []
for i in range(nsec):
    o = pe + 24 + optsz + i * 40
    vs, va, rs, ro = struct.unpack_from('<IIII', d, o + 8)
    secs.append((d[o:o + 8].rstrip(b'\0').decode('latin1'), va, vs, ro, rs))
def rva_of(o):
    for nm, va, vs, ro, rs in secs:
        if ro <= o < ro + rs: return va + (o - ro)
consts, arrays = {}, {}
for f in glob.glob(SRC + '/**/*.*', recursive=True):
    if not f.endswith(('.h', '.cpp')): continue
    s = open(f, encoding='utf-8', errors='replace').read()
    rel = os.path.relpath(f, SRC).replace(os.sep, '/')
    for m in re.finditer(r'constexpr\s+uintptr_t\s+(k\w+)\s*=\s*(0x[0-9A-Fa-f]+)', s):
        consts.setdefault(m.group(1), []).append((int(m.group(2), 16), rel))
    for m in re.finditer(r'constexpr\s+(?:std::)?uint8_t\s+(k\w+)\[\]\s*=\s*\{([^}]*)\}', s):
        arrays[m.group(1)] = bytes(int(x, 16) for x in re.findall(r'0x([0-9A-Fa-f]{1,2})', m.group(2)))
out = {}
stats = {'unique': 0, 'multi': 0, 'none': 0, 'nohead': 0}
for name, defs in sorted(consts.items()):
    old, rel = defs[0]
    head = None
    for suf in ('Head', 'Bytes', 'Sig', 'Code', 'Prologue'):
        if name + suf in arrays: head = arrays[name + suf]; break
    e = {'old': old, 'file': rel}
    if head is None or len(head) < 6:
        e['status'] = 'nohead'; stats['nohead'] += 1
    else:
        hits, i = [], d.find(head)
        while i >= 0 and len(hits) < 8:
            hits.append(rva_of(i)); i = d.find(head, i + 1)
        e['hits'] = hits; e['headlen'] = len(head)
        e['status'] = 'unique' if len(hits) == 1 else 'multi' if hits else 'none'
        stats[e['status']] += 1
    out[name] = e
json.dump(out, open('heads.json', 'w'), indent=1)
print(stats, 'of', len(consts))
