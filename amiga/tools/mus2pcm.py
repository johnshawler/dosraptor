#!/usr/bin/env python3
"""Render the DMX MUS songs of the supplied GLB files to 8-bit signed 16 kHz PCM, the way DMX plays them on an OPL2 (GENMIDI instruments).
   usage: mus2pcm.py DATADIR OUTDIR     (needs numpy, scipy, pyopl)"""
import os, struct, sys, math
import numpy as np
from scipy.signal import resample_poly
import pyopl
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..'))
from tools_glbdump import items, decrypt

RATE_OPL = 48000; RATE_OUT = 16000; TICK = 140
OPS = [0, 1, 2, 8, 9, 10, 16, 17, 18]

def load_all(data):
    out = {}
    for fn in sorted(os.listdir(data)):
        if not fn.upper().endswith('.GLB'): continue
        d, res = items(os.path.join(data, fn))
        for i, name, opt, off, size in res:
            b = d[off:off + size]
            if opt == 1: b = decrypt(b)
            out.setdefault(name.rstrip('/ '), b)
    return out

class Instr:
    def __init__(self, b):
        self.flags, self.fine, self.fixed = struct.unpack('<HBB', b[:4])
        self.v = []
        for k in range(2):
            v = b[4 + k * 16: 20 + k * 16]
            self.v.append(dict(mod=v[0:6], fb=v[6], car=v[7:13], base=struct.unpack('<h', v[14:16])[0]))

def parse_genmidi(b):
    assert b[:8] == b'#OPL_II#'
    return [Instr(b[8 + i * 36: 8 + (i + 1) * 36]) for i in range(175)]

VT = [round(127 * (v / 127.0) ** 0.8) for v in range(128)]

class Chan:
    def __init__(s): s.instr = 0; s.vol = 100; s.bend = 128; s.sustain = 0

class Voice:
    def __init__(s): s.ch = None; s.note = -1; s.key = -1; s.on = False; s.age = 0; s.vel = 0; s.ins = None; s.vi = 0; s.fnum = 0; s.blk = 0

class Player:
    def __init__(s, genmidi, nsamp_cb):
        s.g = genmidi; s.opl = pyopl.opl(RATE_OPL, 2, 1); s.chs = [Chan() for _ in range(16)]; s.vs = [Voice() for _ in range(9)]
        s.age = 0; s.out = []; s.frac = 0.0; s.w(1, 0x20); s.w(0xBD, 0)
    def w(s, r, v): s.opl.writeReg(r, v & 255)
    def run(s, seconds):
        s.frac += seconds * RATE_OPL; n = int(s.frac); s.frac -= n
        if n <= 0: return
        while n > 0:
            k = min(n, 512); buf = bytearray(2 * k); s.opl.getSamples(buf); s.out.append(np.frombuffer(bytes(buf), dtype='<i2').copy()); n -= k
    def setvol(s, v):
        ins = v.ins; vd = ins.v[v.vi]; ch = v.ch
        vs = (VT[v.vel] / 127.0) * ((2 + (VT[ch.vol] >> 1)) / 65.0)
        car = vd['car']; mod = vd['mod']; n = OPS[s.vs.index(v)]
        catt = 63 - round((63 - (car[5] & 63)) * vs)
        s.w(0x40 + n + 3, (car[4] & 0xC0) | catt)
        if vd['fb'] & 1:
            matt = 63 - round((63 - (mod[5] & 63)) * vs)
            s.w(0x40 + n, (mod[4] & 0xC0) | matt)
        else: s.w(0x40 + n, (mod[4] & 0xC0) | (mod[5] & 63))
    def setinstr(s, v):
        vd = v.ins.v[v.vi]; n = OPS[s.vs.index(v)]; vi = s.vs.index(v)
        for op, t in ((n, vd['mod']), (n + 3, vd['car'])):
            s.w(0x20 + op, t[0]); s.w(0x60 + op, t[1]); s.w(0x80 + op, t[2]); s.w(0xE0 + op, t[3])
        s.w(0xC0 + vi, vd['fb'] & 0x0F)
    def freq(s, v):
        ins = v.ins; vd = ins.v[v.vi]; ch = v.ch
        note = ins.fixed if ins.flags & 1 else v.note
        if not (ins.flags & 1) or True: note += vd['base'] if not (ins.flags & 1) else 0
        while note < 0: note += 12
        while note > 95: note -= 12
        bend = (ch.bend - 128) / 128.0 * 2.0
        f = 440.0 * 2 ** ((note + bend - 69) / 12.0)
        for blk in range(8):
            fnum = f * (1 << (20 - blk)) / 49716.0
            if fnum < 1024: break
        fnum = min(1023, int(round(fnum))); v.fnum = fnum; v.blk = blk
        vi = s.vs.index(v)
        s.w(0xA0 + vi, fnum & 255); s.w(0xB0 + vi, (0x20 if v.on else 0) | (blk << 2) | (fnum >> 8))
    def alloc(s):
        for v in s.vs:
            if not v.on and v.ch is None: return v
        free = [v for v in s.vs if not v.on]
        if free: return min(free, key=lambda x: x.age)
        return min(s.vs, key=lambda x: x.age)
    def note_on(s, c, note, vel):
        ch = s.chs[c]
        if c == 15:
            idx = 128 + max(0, min(46, note - 35))
        else: idx = ch.instr
        ins = s.g[idx]
        for vi in range(2 if ins.flags & 4 else 1):
            v = s.alloc(); n = s.vs.index(v)
            if v.on: s.w(0xB0 + n, 0)
            s.age += 1; v.age = s.age; v.ch = ch; v.note = note; v.key = c; v.on = True; v.vel = vel; v.ins = ins; v.vi = vi
            s.setinstr(v); s.setvol(v); s.freq(v)
    def note_off(s, c, note):
        for v in s.vs:
            if v.on and v.key == c and v.note == note:
                v.on = False; s.w(0xB0 + s.vs.index(v), (v.blk << 2) | (v.fnum >> 8))
    def play(s, mus):
        ln, st = struct.unpack('<HH', mus[4:8]); p = st; lastvol = [0] * 16
        while p < len(mus):
            ev = mus[p]; p += 1; last = ev & 0x80; t = (ev >> 4) & 7; c = ev & 15
            if t == 0:
                s.note_off(c, mus[p] & 127); p += 1
            elif t == 1:
                n = mus[p]; p += 1
                if n & 0x80: lastvol[c] = mus[p] & 127; p += 1
                s.note_on(c, n & 127, lastvol[c])
            elif t == 2:
                s.chs[c].bend = mus[p]; p += 1
                for v in s.vs:
                    if v.on and v.key == c: s.freq(v)
            elif t == 3: p += 1
            elif t == 4:
                ctl, val = mus[p], mus[p + 1]; p += 2
                if ctl == 0: s.chs[c].instr = min(val, 127)
                elif ctl == 3:
                    s.chs[c].vol = val & 127
                    for v in s.vs:
                        if v.on and v.key == c: s.setvol(v)
            elif t == 6: break
            if last:
                d = 0
                while True:
                    b = mus[p]; p += 1; d = d * 128 + (b & 127)
                    if not b & 128: break
                s.run(d / TICK)
        s.run(1.0)                                   # release tail
        return np.concatenate(s.out) if s.out else np.zeros(1, np.int16)

if __name__ == '__main__':
    data, outdir = sys.argv[1], sys.argv[2]; os.makedirs(outdir, exist_ok=True)
    allitems = load_all(data); g = parse_genmidi(allitems['GENMIDI_OP2'])
    pcm = {}
    for name, b in allitems.items():
        if name.endswith('_MUS') and b[:4] == b'MUS\x1a':
            x = Player(g, None).play(b).astype(np.float64) / 32768.0
            pcm[name] = resample_poly(x, 1, RATE_OPL // RATE_OUT)
            print(name, '%.1fs' % (len(pcm[name]) / RATE_OUT), 'peak %.2f' % np.abs(pcm[name]).max())
    # per-song loudness: remove DC, aim for the same RMS, never clip (a global gain left the quiet songs at 8-bit noise level)
    for name, x in pcm.items():
        x = x - x.mean(); rms = math.sqrt((x ** 2).mean()) or 1e-9; pk = np.abs(x).max() or 1e-9
        g = min(0.20 / rms, 0.95 / pk); pcm[name] = x * g
    gain = 1.0
    for name, x in pcm.items():
        out = np.clip(np.round(x * gain * 127), -128, 127).astype(np.int8)
        open(os.path.join(outdir, name[:-4] + '.PCM'), 'wb').write(out.tobytes())
    import zlib
    with open(os.path.join(outdir, 'INDEX.TXT'), 'w') as f:
        for name in pcm:
            b = allitems[name]; ln, st = struct.unpack('<HH', b[4:8]); n = ln + st
            f.write('%d %08x %s\n' % (n, zlib.crc32(b[:n]) & 0xffffffff, name[:-4]))
    print('gain %.2f' % gain)
