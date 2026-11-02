"""Crimson Desert save.save container: decode to the raw blob and encode it back (test saves only).

Layout (header 0x80 bytes, little endian), from NattKh's CrimsonSaveEditor (save_crypto.py, GPL) and checked on our
own saves:
  +0x00 "SAVE" | +0x04 u16 version (2) | +0x06 u16 flags (0x0080) | +0x12 u32 uncompressed size
  +0x16 u32 payload size | +0x1A 16-byte ChaCha20 nonce (first u32 = block counter) | +0x2A HMAC-SHA256 (32 bytes)
  +0x80 payload = ChaCha20(LZ4 block(blob)); the HMAC covers the LZ4 block (before encryption).
The key is a fixed 31-byte XOR of a base key and a version string, plus a zero byte.

Pure Python (no lz4/cryptography modules needed). Our compressor is a simple greedy LZ4 one.

  python tools/save/cdsave.py decode <save.save> <out.bin>
  python tools/save/cdsave.py encode <blob.bin> <template save.save> <out save.save>
  python tools/save/cdsave.py check <save.save>
"""
import hashlib
import hmac
import os
import struct
import sys

_BASE = bytes.fromhex("C41B8E730DF259A637CC04E9B12F9668DA107A853E61F9224DB80AD75C13EF90")[:31]
_PREFIX = {1: b'^Qgbrm/.#@`zsr]\\@rvfal#"', 2: b"^Pearl--#Abyss__@!!"}


def key_for(version):
    material = _PREFIX[version] + b"PRIVATE_HMAC_SECRET_CHECK"
    return bytes(x ^ y for x, y in zip(_BASE, material)) + b"\x00"


def _rotl(v, n):
    return ((v << n) | (v >> (32 - n))) & 0xFFFFFFFF


def chacha20(data, nonce16, key):
    kw = struct.unpack("<8I", key)
    counter = struct.unpack_from("<I", nonce16, 0)[0]
    nw = struct.unpack_from("<3I", nonce16, 4)
    out = bytearray(len(data))
    M = 0xFFFFFFFF
    for pos in range(0, len(data), 64):
        s = [0x61707865, 0x3320646E, 0x79622D32, 0x6B206574, *kw, counter & M, *nw]
        w = s[:]
        for _ in range(10):
            for a, b, c, d in ((0, 4, 8, 12), (1, 5, 9, 13), (2, 6, 10, 14), (3, 7, 11, 15),
                               (0, 5, 10, 15), (1, 6, 11, 12), (2, 7, 8, 13), (3, 4, 9, 14)):
                w[a] = (w[a] + w[b]) & M; w[d] = _rotl(w[d] ^ w[a], 16)
                w[c] = (w[c] + w[d]) & M; w[b] = _rotl(w[b] ^ w[c], 12)
                w[a] = (w[a] + w[b]) & M; w[d] = _rotl(w[d] ^ w[a], 8)
                w[c] = (w[c] + w[d]) & M; w[b] = _rotl(w[b] ^ w[c], 7)
        block = struct.pack("<16I", *[(w[i] + s[i]) & M for i in range(16)])
        chunk = data[pos:pos + 64]
        out[pos:pos + len(chunk)] = bytes(x ^ y for x, y in zip(chunk, block))
        counter += 1
    return bytes(out)


def lz4_decompress(src, size):
    out = bytearray()
    i = 0
    n = len(src)
    while i < n:
        tok = src[i]; i += 1
        lit = tok >> 4
        if lit == 15:
            while True:
                b = src[i]; i += 1; lit += b
                if b != 255:
                    break
        out += src[i:i + lit]; i += lit
        if i >= n:
            break
        off = src[i] | (src[i + 1] << 8); i += 2
        ml = tok & 15
        if ml == 15:
            while True:
                b = src[i]; i += 1; ml += b
                if b != 255:
                    break
        ml += 4
        start = len(out) - off
        if off >= ml:
            out += out[start:start + ml]
        else:
            for k in range(ml):
                out.append(out[start + k])
    if len(out) != size:
        raise ValueError(f"LZ4: got {len(out)} bytes, expected {size}")
    return bytes(out)


def _len_bytes(v):
    out = bytearray()
    while v >= 255:
        out.append(255); v -= 255
    out.append(v)
    return out


def lz4_compress(src):
    """Greedy LZ4 block (hash of 4 bytes, 64 KB window). Last 5 bytes are literals, as the format requires."""
    n = len(src)
    out = bytearray()
    table = {}
    anchor = 0
    i = 0
    limit = n - 12
    while i < limit:
        seq = src[i:i + 4]
        cand = table.get(seq)
        table[seq] = i
        if cand is None or i - cand > 0xFFFF:
            i += 1
            continue
        ml = 4
        maxm = n - 5 - i
        while ml < maxm and src[cand + ml] == src[i + ml]:
            ml += 1
        lit = i - anchor
        tok = (min(lit, 15) << 4) | min(ml - 4, 15)
        out.append(tok)
        if lit >= 15:
            out += _len_bytes(lit - 15)
        out += src[anchor:i]
        out += struct.pack("<H", i - cand)
        if ml - 4 >= 15:
            out += _len_bytes(ml - 4 - 15)
        i += ml
        anchor = i
    lit = n - anchor
    out.append(min(lit, 15) << 4)
    if lit >= 15:
        out += _len_bytes(lit - 15)
    out += src[anchor:]
    return bytes(out)


def decode(path):
    d = open(path, "rb").read()
    if d[:4] != b"SAVE":
        raise ValueError("not a SAVE file")
    ver = struct.unpack_from("<H", d, 4)[0]
    usize, psize = struct.unpack_from("<II", d, 0x12)
    nonce = d[0x1A:0x2A]
    mac = d[0x2A:0x4A]
    key = key_for(ver)
    comp = chacha20(d[0x80:0x80 + psize], nonce, key)
    if not hmac.compare_digest(hmac.new(key, comp, hashlib.sha256).digest(), mac):
        raise ValueError("HMAC mismatch")
    return d[:0x80], lz4_decompress(comp, usize)


def encode(blob, header):
    ver = struct.unpack_from("<H", header, 4)[0]
    key = key_for(ver)
    comp = lz4_compress(blob)
    nonce = os.urandom(16)
    h = bytearray(header[:0x80])
    struct.pack_into("<II", h, 0x12, len(blob), len(comp))
    h[0x1A:0x2A] = nonce
    h[0x2A:0x4A] = hmac.new(key, comp, hashlib.sha256).digest()
    return bytes(h) + chacha20(comp, nonce, key)


if __name__ == "__main__":
    cmd = sys.argv[1]
    if cmd == "decode":
        _, blob = decode(sys.argv[2])
        open(sys.argv[3], "wb").write(blob)
        print(f"decoded {len(blob)} bytes")
    elif cmd == "check":
        _, blob = decode(sys.argv[2])
        print(f"OK {len(blob)} bytes")
    elif cmd == "encode":
        header, _ = decode(sys.argv[3])
        out = encode(open(sys.argv[2], "rb").read(), header)
        open(sys.argv[4], "wb").write(out)
        _, back = decode(sys.argv[4])
        assert back == open(sys.argv[2], "rb").read()
        print(f"encoded {len(out)} bytes (round trip OK)")
