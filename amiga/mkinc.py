#!/usr/bin/env python3
"""Regenerate FILE000x.INC for the GLB files actually supplied (the shareware data set numbers its items differently from the
registered-version tables in the source).  usage: mkinc.py DATADIR inc_in inc_out FILEnnnn..."""
import re, sys, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from tools_glbdump import items
data = sys.argv[1]; names = {}
for fn in os.listdir(data):
    m = re.match(r'FILE(\d{4})\.GLB$', fn, re.I)
    if not m: continue
    _, res = items(os.path.join(data, fn)); fno = int(m.group(1))
    for i, name, opt, off, size in res:
        names.setdefault(name, []).append((fno, i))
out = []; missing = []
for src, dst in zip(sys.argv[2::2], sys.argv[3::2]):
    own = int(re.search(r'FILE(\d{4})', os.path.basename(src), re.I).group(1))
    lines = []
    for l in open(src, encoding='latin1'):
        m = re.match(r'(#define\s+)(\S+)(\s+)0x([0-9a-fA-F]+)(.*)', l)
        if m:
            c = names.get(m.group(2))
            if c:
                f, i = next((x for x in c if x[0] == own), c[0])
                l = '%s%s%s0x%08x%s\n' % (m.group(1), m.group(2), m.group(3), (f << 16) | i, m.group(5))
            else:
                missing.append(m.group(2)); l = '%s%s%s0xffffffff%s\n' % (m.group(1), m.group(2), m.group(3), m.group(5))
        lines.append(l.rstrip('\n') + '\n')
    open(dst, 'w', encoding='latin1').write(''.join(lines))
print('mkinc: %d names absent from the supplied data' % len(missing), file=sys.stderr)
