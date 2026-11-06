import sys
from capstone import Cs, CS_ARCH_X86, CS_MODE_64
from pe49 import rd
md = Cs(CS_ARCH_X86, CS_MODE_64)
def dis(rva, n=60, size=0x400):
    out = []
    for i, ins in enumerate(md.disasm(rd(rva, size), rva)):
        out.append('%8X  %-8s %s' % (ins.address, ins.mnemonic, ins.op_str))
        if i + 1 >= n: break
    return out
if __name__ == '__main__':
    print('\n'.join(dis(int(sys.argv[1], 16), int(sys.argv[2]) if len(sys.argv) > 2 else 60)))
