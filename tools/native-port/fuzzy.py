# Find each address constant of a native source file in the 3549 exe by its recorded byte array, with every
# RIP-relative displacement and rel32 call/jump/jcc target wildcarded (they move between builds; the rest of the
# code usually does not).
#
#   python fuzzy.py <file under src/mod>            every `kName` with a `kName<Bytes|Head|...>[]` array
#   python fuzzy.py <file> kName [kName ...]          only these
#
# A unique hit is a strong candidate, not proof: check the callers / vtable slots / neighbours before applying it
# (addport.py), and keep the evidence in docs/research/native-port-3549.json.
import re, sys
from capstone import Cs, CS_ARCH_X86, CS_MODE_64
from pe49 import d, rva_of, secs

SRC = 'C:/Users/asher/CrimsonDesert-SeamlessCoop/upstream/cdmp-native/src/mod/'
SUFFIXES = ('Bytes', 'Head', 'HeadBytes', 'Aob', 'Sig', 'PrologueBytes')
COMMENT = re.compile(r'//[^\r\n]*')
HEXBYTE = re.compile(r'0x([0-9A-Fa-f]{1,2})\b')
md = Cs(CS_ARCH_X86, CS_MODE_64)
md.detail = True
tx = [s for s in secs if s[0] == '.xtext'][0]
TEXT_LO, TEXT_HI, TEXT_RVA = tx[3], tx[3] + tx[4], tx[1]  # raw file range and RVA of .xtext


def masked_pattern(code, at):
    """Regex over `code` (located at RVA `at` in the old build) with moving fields as wildcards."""
    keep = [True] * len(code)
    pos = 0
    for ins in md.disasm(code, at):
        pos = ins.address - at + ins.size
        off = ins.address - at
        for op in ins.operands:
            if op.type == 3 and op.mem.base == 41 and ins.disp_size:  # X86_REG_RIP = 41
                for i in range(ins.disp_offset, ins.disp_offset + ins.disp_size): keep[off + i] = False
        if ins.group(2) or ins.group(1) or ins.group(7):  # call / jump / branch-relative
            if ins.imm_size == 4:
                for i in range(ins.imm_offset, ins.imm_offset + 4): keep[off + i] = False
    return b''.join(re.escape(bytes([b])) if k else b'.' for b, k in zip(code, keep)), pos


def search(code, at):
    pat, decoded = masked_pattern(code, at)
    return [TEXT_RVA + m.start() for m in re.finditer(pat, d[TEXT_LO:TEXT_HI], re.S)], decoded


def constants(text):
    out = {}
    for m in re.finditer(r'constexpr\s+uintptr_t\s+(k\w+)\s*=\s*(0x[0-9A-Fa-f]+)', text):
        name, val = m.group(1), int(m.group(2), 16)
        best = b''
        for suf in SUFFIXES:
            am = re.search(r'constexpr\s+(?:std::)?uint8_t\s+' + name + suf + r'\[\]\s*=\s*\{([^}]*)\}', text)
            if am:
                b = bytes(int(x, 16) for x in HEXBYTE.findall(COMMENT.sub('', am.group(1))))
                if len(b) > len(best): best = b
        out[name] = (val, best)
    return out


if __name__ == '__main__':
    text = open(SRC + sys.argv[1], encoding='utf-8').read()
    want = set(sys.argv[2:])
    for name, (old, code) in constants(text).items():
        if want and name not in want: continue
        if not code:
            print('%-34s 0x%-8X no byte array' % (name, old)); continue
        hits, decoded = search(code, old)
        tag = 'UNIQUE' if len(hits) == 1 else ('none' if not hits else '%d hits' % len(hits))
        print('%-34s 0x%-8X %3d B (%3d decoded) %-8s %s' % (name, old, len(code), decoded, tag,
                                                     ' '.join('0x%X' % h for h in hits[:6])))
