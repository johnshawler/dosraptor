#!/bin/sh
# Lowercase mirror of the case-insensitive DOS source tree (build/src), so #include "raptor.h" resolves on Linux.
cd "$(dirname "$0")/.."; M=amiga/build/src; rm -rf $M; mkdir -p $M
for d in SOURCE GFX apodmx audiolib; do for f in $d/*; do [ -f "$f" ] || continue
  b=$(basename "$f" | tr A-Z a-z); case $d in SOURCE) ;; *) [ -e $M/$b ] && continue;; esac
  tr -d "\032\r" < "$f" | sed -E '/^[[:space:]]*#[[:space:]]*include/ s#\\#/#g' > $M/$b; done; done
sed -i "/^int random(int);/d" $M/types.h
# --- compatibility patches (Watcom accepted these; GCC does not) ---
cd $M
sed -i '/^extern CONTROL control;/d' fx.h
python3 - <<'PY'
import re
s=open('objects.h').read()
for n in ('OBJS_Add','OBJS_Buy'):
    s=re.sub(r'BOOL([^\n]*\n'+n+r' \()',r'BUYSTUFF\1',s)
open('objects.h','w').write(s)
PY
sed -i 's/^PUBLIC INT buttons\[4\]/PUBLIC BOOL buttons[4]/' input.c
sed -i 's/   GFLAG groundflag/   BOOL  groundflag/' anims.c
sed -i 's/^BUYSTUFF *$/BUYSTUFF/' objects.c
python3 - <<'PY'
import re
s=open('tile.c').read()
s=re.sub(r'BOOL([^\n]*\nTILE_IsHit \()',r'T_HIT\1',s); open('tile.c','w').write(s)
PY
