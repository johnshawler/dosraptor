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
python3 - <<'PY'
import re
s=open('kbdapi.c').read()
s=s.replace("PRIVATE VOID ( _interrupt _far * oldkeyboardisr )()  = 0L;","PRIVATE void * oldkeyboardisr = 0L;")
s=s.replace("SPECIAL VOID _interrupt\nKBD_ReadScan(void)","VOID\nKBD_ReadScan(int key)")
s=s.replace("   static BYTE key;\n","").replace("   key = inp ( 0x60 );\n","").replace("   outp( 0x20, 0x20 );\n","")
s=s.replace("            *(short *)0x41c = *(short *)0x41a; // clear bios key buffer\n","")
s=s.replace("_dos_setvect ( KEYBOARDINT, KBD_ReadScan );","")
open('kbdapi.c','w').write(s)
g=open('gfxapi.c').read()
g=g.replace("PUBLIC   BYTE *   displayscreen  = (BYTE *)0xa0000;","extern unsigned char *plat_vram;\nvoid plat_present(void);\nPUBLIC   BYTE *   displayscreen  = 0;")
g=g.replace("   displayscreen = (BYTE *)0xa0000;\n}","   displayscreen = plat_vram;\n}")
g=g.replace("   hold = FRAME_COUNT;\n}","   plat_present();\n   hold = FRAME_COUNT;\n}")
open('gfxapi.c','w').write(g)
PY
sed -i 's/^PRIVATE INT       active_field/PUBLIC  INT       active_field/' swdapi.c
: > regver.h   # shareware data set (FILE0000/FILE0001 only): build the unregistered variant
python3 - <<'PY'
s=open('glbapi.c').read()
s=s.replace('PRIVATE CHAR *    serial = "32768GLB";','PRIVATE CHAR *    serial = "32768GLB";\nextern void GLB_SwapItem(const char *name, BYTE *p, unsigned size);\nstatic DWORD sw32k(DWORD v){return (v>>24)|((v>>8)&0xff00)|((v<<8)&0xff0000)|(v<<24);}')
s=s.replace("   return ( ( int ) key.offset );","   return ( ( int ) sw32k ( key.offset ) );")
s=s.replace("      	if ( key[ n ].opt == GLB_ENCODED )","      	key[ n ].opt = sw32k ( key[ n ].opt ); key[ n ].offset = sw32k ( key[ n ].offset ); key[ n ].filesize = sw32k ( key[ n ].filesize );\n      	if ( key[ n ].opt == GLB_ENCODED )")
s=s.replace("         \tGLB_DeCrypt( serial, inmem, ii->size );\n      \t}\n#endif\n","         \tGLB_DeCrypt( serial, inmem, ii->size );\n      \t}\n#endif\n      \tGLB_SwapItem( ii->name, inmem, ii->size );\n")
open('glbapi.c','w').write(s)
PY
python3 - <<'PY'
s=open('glbapi.c').read()
s=s.replace("   WORD     itemnum;\n   WORD     filenum;\n} ITEM_ID;","   WORD     filenum;   /* big endian: high half of the handle */\n   WORD     itemnum;\n} ITEM_ID;")
open('glbapi.c','w').write(s)
PY
sed -i 's/EXIT_Error ( "GLB_FetchItem: failed on %d bytes, mode=%d.", ii->size, mode );/EXIT_Error ( "GLB_FetchItem: failed on %d bytes, mode=%d handle=%08x.", ii->size, mode, handle );/' glbapi.c
cd ../..   # amiga/
RD=${RAPTOR_DATA:-/tmp/claude-0/-home-user/387e5900-dc29-591a-9a5a-61a50108a5bd/scratchpad/rz/raptor}
./mkinc.py "$RD" build/src/file0000.inc build/src/file0000.inc build/src/file0001.inc build/src/file0001.inc
cd build/src
python3 - <<'PY'
s=open('glbapi.c').read()
open('glbapi.c','w').write(s)
PY
cd ../..
cd build/src
python3 - <<'PY'
s=open('glbapi.c').read()
open('glbapi.c','w').write(s)
PY
cd ../..
