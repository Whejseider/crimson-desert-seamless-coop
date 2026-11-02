"""Read cdmp_packets_*.bin (troc_sniffer.cpp writer): yields (t_s, dir, data) for every recorded packet.
  python tools/port/pkbin.py file.bin [op ...]   prints the packets with those opcodes (hex), whole."""
import struct
import sys


def records(path):
    b = open(path, 'rb').read()
    assert b[:4] == b'CDPK', 'not a cdmp packet recording'
    at = 12
    while at + 24 <= len(b):
        t_us, d, _, size, copied, _, _, _ = struct.unpack_from('<QBBHHHII', b, at)
        at += 24
        yield t_us / 1e6, d, b[at:at + copied]
        at += copied


if __name__ == '__main__':
    ops = {int(x) for x in sys.argv[2:]}
    for t, d, data in records(sys.argv[1]):
        op = struct.unpack_from('<H', data, 0)[0] if len(data) >= 2 else -1
        if not ops or op in ops:
            print('%.3f %d %d %s' % (t, d, op, data.hex()))
