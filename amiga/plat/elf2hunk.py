#!/usr/bin/env python3
"""Convert a statically linked big-endian m68k ELF (linked with link.ld, --emit-relocs) to an AmigaOS hunk executable."""
import struct, sys
elf = open(sys.argv[1], 'rb').read()
(e_shoff,) = struct.unpack('>I', elf[32:36]); e_shentsize, e_shnum, e_shstrndx = struct.unpack('>HHH', elf[46:52])
sh = []
for i in range(e_shnum):
    n, t, fl, ad, off, sz, lk, inf, al, es = struct.unpack('>IIIIIIIIII', elf[e_shoff+i*e_shentsize:][:40]); sh.append((n,t,fl,ad,off,sz,lk,inf))
BSS = 0x10000000
code = bytearray(); bss_end = BSS
alloc = [(ad, off, sz, t) for (n,t,fl,ad,off,sz,lk,inf) in sh if fl & 2 and sz]
cend = 0
for ad, off, sz, t in alloc:
    if ad < BSS:
        cend = max(cend, ad + sz)
code = bytearray(cend)
for ad, off, sz, t in alloc:
    if ad < BSS:
        if t != 8: code[ad:ad+sz] = elf[off:off+sz]
    else: bss_end = max(bss_end, ad + sz)
while len(code) % 4: code.append(0)
bsssize = (bss_end - BSS + 3) & ~3
rel = {0: [], 1: []}
for (n,t,fl,ad,off,sz,lk,inf) in sh:
    if t != 4: continue
    for i in range(sz // 12):
        o, info, add = struct.unpack('>IIi', elf[off+i*12:][:12]); ty = info & 255
        if o >= cend: continue      # relocation inside bss: nothing
        if ty == 1:                 # R_68K_32 absolute
            (v,) = struct.unpack('>I', code[o:o+4])
            if v >= BSS: rel[1].append(o); code[o:o+4] = struct.pack('>I', v - BSS)
            else: rel[0].append(o)
        elif ty in (4, 5, 6, 7, 8, 9, 10, 11, 12):  # PC32, PC16, PC8 and GOT forms: unsupported unless same hunk
            if ty not in (4, 5, 6): sys.exit('unsupported reloc %d at %x' % (ty, o))
out = bytearray()
def w(*a): out.extend(struct.pack('>%dI' % len(a), *a))
w(0x3F3, 0, 2, 0, 1, len(code)//4, bsssize//4)
w(0x3E9, len(code)//4); out.extend(code)
if rel[0] or rel[1]:
    w(0x3EC)
    for h in (0, 1):
        if rel[h]:
            w(len(rel[h]), h); w(*sorted(rel[h]))
    w(0)
w(0x3F2, 0x3EB, bsssize//4, 0x3F2)
open(sys.argv[2], 'wb').write(out)
print('code %d bytes, bss %d bytes, relocs %d/%d' % (len(code), bsssize, len(rel[0]), len(rel[1])))
