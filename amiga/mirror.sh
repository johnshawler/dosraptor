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
s=s.replace("VOID\nKBD_ReadScan(int key)\n{","VOID\nKBD_ReadScan(int key)\n{\n   if ( KBD_TRACE ) isr_log(\"KEY %02x\\n\",key);")
s="#ifndef KBD_TRACE\n#define KBD_TRACE 0\n#endif\nvoid isr_log(const char *f, ...);\n"+s
open('kbdapi.c','w').write(s)
g=open('gfxapi.c').read()
g=g.replace("PUBLIC   BYTE *   displayscreen  = (BYTE *)0xa0000;","extern unsigned char *plat_vram;\nvoid plat_present(void);\nPUBLIC   BYTE *   displayscreen  = 0;")
g=g.replace("   displayscreen = (BYTE *)0xa0000;\n}","   displayscreen = plat_vram;\n}")
g=g.replace("   hold = FRAME_COUNT;\n}","   { static int hb; extern int inject_dbg[4]; if ( HB_TRACE && ++hb % 100 == 0 ) printf(\"HB cur=%d ev=%d wait=%d dl=%d\\n\", inject_dbg[0], inject_dbg[1], inject_dbg[2], inject_dbg[3] ); }\n   hold = FRAME_COUNT;\n}")
g="#ifndef HB_TRACE\n#define HB_TRACE 0\n#endif\n"+g
open('gfxapi.c','w').write(g)
PY
sed -i 's/^PRIVATE INT       active_field/PUBLIC  INT       active_field/' swdapi.c
sed -i 's/^   header = ( SWIN \* )GLB_LockItem ( handle );/   printf("WIN %08x\\n", handle);\n   header = ( SWIN * )GLB_LockItem ( handle );/' swdapi.c
[ -e "${RAPTOR_DATA:?}/FILE0002.GLB" ] || : > regver.h   # shareware data (FILE0000/FILE0001 only): unregistered build; registered data keeps REG_VERSION
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
RD=${RAPTOR_DATA:?set RAPTOR_DATA to the directory holding FILE0000.GLB and FILE0001.GLB}
set --; for n in 0 1 2 3 4; do [ -e "$RD/FILE000$n.GLB" ] && [ -e build/src/file000$n.inc ] && set -- "$@" build/src/file000$n.inc build/src/file000$n.inc; done
./mkinc.py "$RD" "$@"
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
cd build/src
python3 - <<'PY'
s=open('glbapi.c').read()
s=s.replace("	return GLB_FetchItem( handle, FI_LOCK );","	if ( TRACE_GLB ) printf(\"LOCK %08x ra=%x self=%x\\n\", handle, (unsigned)__builtin_return_address(0), (unsigned)GLB_LockItem);\n	return GLB_FetchItem( handle, FI_LOCK );")
for fn,mode in (("GLB_CacheItem","FI_CACHE"),("GLB_GetItem","FI_DISCARD")):
    s=s.replace("	return GLB_FetchItem( handle, %s );"%mode,"	if ( handle == ~0 ) printf(\"EMPTY %s ra=%%x self=%%x\\n\", (unsigned)__builtin_return_address(0), (unsigned)%s);\n	return GLB_FetchItem( handle, %s );"%(fn,fn,mode))
s=s.replace("	if ( TRACE_GLB ) printf(","	if ( handle == ~0 ) printf(\"EMPTY GLB_LockItem ra=%x self=%x\\n\", (unsigned)__builtin_return_address(0), (unsigned)GLB_LockItem);\n	if ( TRACE_GLB ) printf(")
s=s.replace("#define _SCOTTGAME","#define _SCOTTGAME\n#ifndef TRACE_GLB\n#define TRACE_GLB 0\n#endif")
open('glbapi.c','w').write(s)
PY
cd ../..
cd build/src
python3 - <<'PY'
import re
def mark(fn, func, tag):
    s=open(fn).read()
    pat=re.compile(r'(\n'+func+r'\s*\([^)]*\)\s*\{)')
    m=pat.search(s)
    if not m: print('mark: not found', func); return
    s=s[:m.end()]+'\n   printf("STATE %s\\n");'%tag+s[m.end():]
    open(fn,'w').write(s)
s=open('swdapi.c').read()
s=s.replace("   KBD_LASTSCAN = SC_NONE;\n   KBD_LASTASCII = SC_NONE;\n   _enable();","   KBD_LASTSCAN = SC_NONE;\n   KBD_LASTASCII = SC_NONE;\n   _enable();\n   plat_idle();\n   { static int hb2; extern int inject_dbg[4]; if ( HB_TRACE && ++hb2 % 20000 == 0 ) { dbg_flush(); printf(\"HB cur=%d ev=%d wait=%d dl=%d\\n\", inject_dbg[0], inject_dbg[1], inject_dbg[2], inject_dbg[3] ); }}\n   if ( g_key && DLG_TRACE ) printf(\"SWDKEY %x %x af=%d kb=%d aw=%d\\n\", g_key, g_ascii, active_field, kbactive, active_window );",1)
s="void dbg_flush(void);\nvoid plat_idle(void);\n#ifndef HB_TRACE\n#define HB_TRACE 0\n#endif\n#ifndef DLG_TRACE\n#define DLG_TRACE 0\n#endif\n"+s
open('swdapi.c','w').write(s)
s=open('ptrapi.c').read()
s=s.replace("      cursorx = dm_x;\n      cursory = dm_y;\n      not_in_update = TRUE;","      plat_present_rect ( cursorx, cursory, 16, 16 );\n      plat_present_rect ( dm_x, dm_y, 16, 16 );\n      cursorx = dm_x;\n      cursory = dm_y;\n      not_in_update = TRUE;")
s="void plat_present_rect(int,int,int,int);\n"+s
s=s.replace("   if ( not_in_update )\n   {\n      cur_mx = m_cx&0xffff;","   if ( MOUSE_TRACE && ( m_bx != mouseb1 ) ) isr_log(\"MOUSE b=%d x=%d y=%d niu=%d\\n\", m_bx, m_cx>>1, m_dx, not_in_update );\n   if ( not_in_update )\n   {\n      cur_mx = m_cx&0xffff;",1)
s="void isr_log(const char *f, ...);\n#ifndef MOUSE_TRACE\n#define MOUSE_TRACE 0\n#endif\n"+s
open('ptrapi.c','w').write(s)
mark('store.c','STORE_Enter','STORE')
mark('windows.c','WIN_Hangar','HANGAR')
mark('windows.c','WIN_ShipComp','SHIPCOMP')
mark('windows.c','WIN_LoadComp','LOADCOMP')
mark('windows.c','WIN_MainLoop','MAINLOOP')
mark('loadsave.c','RAP_LoadMap','LOADMAP')
mark('rap.c','Do_Game','GAME')
PY
cd ../..
cd build/src
[ "$NOFLUSH" = 1 ] || sed -i 's/while ( *PTR_B1 *);/while ( PTR_B1 ) dbg_flush();/; s/while ( *SWD_IsButtonDown() *);/while ( SWD_IsButtonDown() ) dbg_flush();/' swdapi.c windows.c store.c rap.c input.c help.c loadsave.c intro.c 2>/dev/null
for f in swdapi.c windows.c store.c rap.c input.c help.c loadsave.c intro.c; do grep -q "dbg_flush" $f && sed -i '1i void dbg_flush(void);' $f; done
cd ../..
cd build/src
python3 - <<'PY'
import re
names=['cur_mx','cur_my','mouseb1','mouseb2','mouseb3','joy2b1','joy2b2','not_in_update','keyboard','lastscan','lastascii','kbd_ack','paused','capslock','mouse_b1_ack','mouse_b2_ack','mouse_b3_ack','mouseaction']
for fn in ('ptrapi.h','ptrapi.c','kbdapi.h','kbdapi.c'):
    s=open(fn).read()
    out=[]
    for line in s.split('\n'):
        if re.match(r'^\s*(extern\s+|PUBLIC\s+|PRIVATE\s+)?(BOOL|INT)\b',line) and not '(' in line.split(';')[0].split('=')[0]:
            for n in names:
                line=re.sub(r'(?<![\w])'+n+r'(?![\w(])','volatile_'+n,line) if False else line
            # insert volatile after the type if any declared name matches
            if any(re.search(r'\b'+n+r'\b',line) for n in names):
                line=re.sub(r'\b(BOOL|INT)\b',r'volatile \1',line,count=1)
        out.append(line)
    open(fn,'w').write('\n'.join(out))
PY
sed -i 's/((volatile)\([A-Za-z_0-9]*\))/(\1)/g; s/!(volatile)not_in_update/!not_in_update/; s/(volatile) *KBD_Key/KBD_Key/' ptrapi.h ptrapi.c swdapi.c
cd ../..
cd build/src
python3 - <<'PY'
s=open('rap.c').read()
s=s.replace("#define MAX_HMEM   ( 4096 * 1024 )","#define MAX_HMEM   ( 4608 * 1024 )   /* 8 MB fast RAM target: VM heap */")
open('rap.c','w').write(s)
g=open('glbapi.c').read()
a=g.index("GLB_DeCrypt(\nCHAR    *key,"); b=g.index("\n}\n",a)+3
g=g[:a]+"""GLB_DeCrypt(
CHAR    *key,               // INPUT : Key that will allow Decryption
BYTE    *buffer,            // INPUT : Buffer to Encrypt
size_t  length              // INPUT : Length of Buffer
)
{
   INT   klen = strlen( key );
   INT   prev_byte;
   INT   kidx;
   const BYTE *k = ( const BYTE * )key;
   BYTE *p = buffer;

   kidx = SEED % klen;
   prev_byte = k[ kidx ];
   while ( length-- )
   {
      INT b = *p;
      *p++ = ( BYTE )( b - k[ kidx ] - prev_byte );
      prev_byte = b;
      if ( ++kidx >= klen )
         kidx = 0;
   }
}
"""+g[b:]
open('glbapi.c','w').write(g)
PY
cd ../..
