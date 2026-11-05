#!/usr/bin/env python3
"""Fill {NAME} placeholders (item ids from build/src/file0001.inc) in an input-script template: mkscript.py template out"""
import re, sys
ids = {}
for l in open('build/src/file0001.inc', encoding='latin1'):
    m = re.match(r'#define\s+(\S+)\s+0x([0-9a-fA-F]+)', l)
    if m: ids[m.group(1)] = '%08x' % int(m.group(2), 16)
open(sys.argv[2], 'w').write(re.sub(r'\{(\w+)\}', lambda m: ids[m.group(1)], open(sys.argv[1]).read()))
