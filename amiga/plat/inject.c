/* Scripted input for automated tests: SYS:inject.txt (optional).  One command per line:
     wait <text>      pause until the game's debug output (dbg.log) contains <text>
     delay <n>        pause n vertical blanks
     key <hex>        press+release a PC set-1 scan code (e.g. 1c = Return)
     text <chars>     type letters/digits/space (a-z 0-9 space)
     mouse <x> <y>    move the pointer (game coordinates 0..319, 0..199)
     click <b>        press and release mouse button b (1 left, 2 right) after moving
   The events enter at the same level as the real hardware input (KBD_ReadScan / the mouse handler). */
#include "os.h"
#include "plat.h"
void *malloc(unsigned);
int strlen(const char *);
int strncmp(const char *, const char *, unsigned);
char *strstr(const char *, const char *);
int atoi(const char *);

#define MAXLINES 400
static char *lines[MAXLINES]; static int nlines, cur;
static int delay; static char waitpat[64]; static int waiting;
static int pending_up = -1, up_delay, pend_btn_up, btn_delay, gap;
static int mx = 160, my = 100;
int inject_dbg[4];
static const char ascmap[] = "\0\0" "1234567890" "\0\0\0\0" "qwertyuiop" "\0\0\0\0" "asdfghjkl" "\0\0\0\0\0" "zxcvbnm";
static int sc_of(char c)
{
	if (c == ' ') return 0x39; if (c == '\n') return 0x1c;
	for (int i = 2; i < (int)sizeof(ascmap) - 1; i++) if (ascmap[i] == c && c) return i;
	return 0;
}
void inject_init(void)
{
	LONG fh = Open("SYS:inject.txt", MODE_OLDFILE);
	if (!fh) return;
	char *buf = malloc(16384); int n = Read(fh, buf, 16383); Close(fh);
	if (n <= 0) return; buf[n] = 0;
	char *p = buf;
	while (*p && nlines < MAXLINES) { lines[nlines++] = p; while (*p && *p != '\n') p++; if (*p) *p++ = 0; }
	waiting = 0; cur = 0;
}
void inject_log(const char *s)
{
	if (waiting && strstr(s, waitpat)) waiting = 0;
}
extern void plat_mouse_event(int, int, int);
extern void KBD_ReadScan(int);
static int typebuf[64], ntype, typepos;
void inject_tick(void)
{
	inject_dbg[0] = cur; inject_dbg[2] = waiting; inject_dbg[3] = delay;
	if (pending_up >= 0 && --up_delay <= 0) { KBD_ReadScan(pending_up | 0x80); pending_up = -1; gap = 10; }
	if (gap > 0) { gap--; return; }
	if (pend_btn_up && --btn_delay <= 0) { plat_mouse_event(0, mx, my); pend_btn_up = 0; }
	if (pending_up >= 0 || pend_btn_up) return;
	if (typepos < ntype) { int sc = typebuf[typepos++]; KBD_ReadScan(sc); pending_up = sc; up_delay = 6; return; }
	if (waiting) return;
	if (delay > 0) { delay--; return; }
	while (cur < nlines) {
		char *l = lines[cur++];
		if (!strncmp(l, "wait ", 5)) { int i = 0; for (const char *q = l + 5; *q && i < 63; q++) waitpat[i++] = *q; waitpat[i] = 0; waiting = 1; return; }
		if (!strncmp(l, "delay ", 6)) { delay = atoi(l + 6); return; }
		if (!strncmp(l, "key ", 4)) { int v = 0; for (const char *q = l + 4; *q; q++) { int d = (*q >= '0' && *q <= '9') ? *q - '0' : (*q >= 'a' && *q <= 'f') ? *q - 'a' + 10 : -1; if (d < 0) break; v = v * 16 + d; } KBD_ReadScan(v); pending_up = v; up_delay = 6; return; }
		if (!strncmp(l, "text ", 5)) { ntype = typepos = 0; for (const char *q = l + 5; *q && ntype < 64; q++) { int sc = sc_of(*q); if (sc) typebuf[ntype++] = sc; } return; }
		if (!strncmp(l, "mouse ", 6)) { const char *q = l + 6; mx = atoi(q); while (*q && *q != ' ') q++; my = atoi(q); plat_mouse_event(0, mx, my); plat_mouse_set(mx, my); inject_dbg[1]++; return; }
		if (!strncmp(l, "click ", 6)) { int b = atoi(l + 6); plat_mouse_event(b, mx, my); pend_btn_up = 1; btn_delay = 15; return; }
	}
}
