#!/usr/bin/env python3
"""Dump the item directory of a Raptor .GLB file (decrypts with the 32768GLB key)."""
import struct, sys
key = b"32768GLB"
def decrypt(buf):
    klen = len(key); kidx = 25 % klen; prev = key[kidx]; out = bytearray()
    for b in buf:
        d = (b - key[kidx] - prev) & 255; prev = b; out.append(d); kidx += 1
        if kidx >= klen: kidx = 0
    return bytes(out)
def items(path):
    f = open(path, 'rb'); data = f.read()
    h = decrypt(data[:28]); n = struct.unpack('<I', h[4:8])[0]; res = []
    for i in range(n):
        e = decrypt(data[28+i*28:28+(i+1)*28]); opt, off, size = struct.unpack('<III', e[:12])
        res.append((i, e[12:].split(b'\0')[0].decode('latin1'), opt, off, size))
    return data, res
if __name__ == '__main__':
    data, res = items(sys.argv[1])
    for r in res: print(*r)
