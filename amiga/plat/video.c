/* Display: AGA 320x200 lores, 8 interleaved bitplanes, own Copper list (the OS display is switched off with LoadView(NULL)).
   The game draws into a 320x200 chunky buffer (plat_vram); plat_present() converts it to planes.  Palette: 256 colours loaded in the
   vertical blank.  Keyboard comes from an Intuition window (OS stays alive), mouse and joystick are read from the custom chips. */
#include "os.h"
#include "plat.h"
int sprintf(char *, const char *, ...);
void inject_init(void);
void c2p_frame(const unsigned char *, unsigned *, unsigned char *);
void c2p_pair(const unsigned char *src, unsigned char *dst);

void *IntuitionBase, *GfxBase;
static void *window, *oldview;
static unsigned char vram_store[320 * 200 + 16];
unsigned char *plat_vram = vram_store;
static unsigned char *planes;                 /* chip RAM: 200 rows x 8 planes x 40 bytes (interleaved by row) */
static unsigned short *cop;                   /* chip RAM copper list */
static unsigned pal24[256];                   /* 0x00RRGGBB */
static volatile int pal_dirty;
static int active, vstart_pal = 1;
static void *chip_ptr;
static int mouse_x = 160, mouse_y = 100, mouse_btn;
extern int plat_is_pal;
#define R16(o) (*(volatile unsigned short *)(0xDFF000 + (o)))

typedef struct __attribute__((packed)) { short x, y, w, h; unsigned char dp, bp; ULONG idcmp, flags; void *gad; void *check; char *title; void *scr; void *bm; short minw, minh, maxw, maxh; unsigned short type; } NewWindow;

void plat_set_color(int i, int r6, int g6, int b6)
{
	unsigned r = (r6 << 2) | (r6 >> 4), g = (g6 << 2) | (g6 >> 4), b = (b6 << 2) | (b6 >> 4);
	pal24[i] = (r << 16) | (g << 8) | b;
	pal_dirty = 1;
}
/* called from the VBL interrupt (before the display window starts) */
void plat_mouse_poll(void)
{
	if (!active) return;
	/* mouse */
	static int primed; static unsigned short last; unsigned short j = R16(0x00A);
	if (!primed) { primed = 1; last = j; }
	signed char dx = (signed char)((j & 0xff) - (last & 0xff)), dy = (signed char)((j >> 8) - (last >> 8));
	last = j;
	if (dx || dy) {
		mouse_x += dx; mouse_y += dy;
		if (mouse_x < 0) mouse_x = 0; if (mouse_x > 319) mouse_x = 319;
		if (mouse_y < 0) mouse_y = 0; if (mouse_y > 199) mouse_y = 199;
	}
	int b = ((*(volatile unsigned char *)0xBFE001 & 0x40) ? 0 : 1) | ((R16(0x016) & 0x400) ? 0 : 2) | ((R16(0x016) & 0x100) ? 0 : 4);
	if (dx || dy || b != mouse_btn) { mouse_btn = b; plat_mouse_event(mouse_btn, mouse_x, mouse_y); }
}
/* Palette loading goes through the Copper: the hardware executes the colour writes at a fixed time at the top of the frame, so a late
   vertical-blank interrupt (the audio interrupt has a higher priority) can no longer spill colour writes into the visible picture, and
   the CPU never fights the Copper over BPLCON3 (colour bank / LOCT select). */
static unsigned short *cop_pal;                 /* palette + base list */
static int pal_frames;
static unsigned short *build_base(unsigned short *p, int vs, int ve, unsigned char *nullspr);
static int g_vs, g_ve; static unsigned char *g_nullspr;
void plat_vbl_hw(void)
{
	if (!active) return;
	if (pal_dirty) {
		pal_dirty = 0;
		unsigned short *p = cop_pal;
		for (int bank = 0; bank < 8; bank++) {
			for (int pass = 0; pass < 2; pass++) {
				*p++ = 0x106; *p++ = 0x0C20 | (bank << 13) | (pass ? 0x200 : 0);
				for (int k = 0; k < 32; k++) {
					unsigned c = pal24[bank * 32 + k];
					unsigned r = (c >> 16) & 255, g = (c >> 8) & 255, bb = c & 255;
					unsigned v = pass ? (((r & 15) << 8) | ((g & 15) << 4) | (bb & 15)) : (((r >> 4) << 8) | ((g >> 4) << 4) | (bb >> 4));
					*p++ = 0x180 + k * 2; *p++ = v;
				}
			}
		}
		*p++ = 0x106; *p++ = 0x0C20;
		p = build_base(p, g_vs, g_ve, g_nullspr);
		*p++ = 0xFFFF; *p++ = 0xFFFE;
		R16(0x080) = (unsigned)cop_pal >> 16; R16(0x082) = (unsigned)cop_pal & 0xffff;
		pal_frames = 2;
	} else if (pal_frames && --pal_frames == 0) {
		R16(0x080) = (unsigned)cop >> 16; R16(0x082) = (unsigned)cop & 0xffff;
	}
}

static unsigned short *cadd(unsigned short *p, int reg, int val) { *p++ = reg; *p++ = val; return p; }
static unsigned short *build_base(unsigned short *p, int vs, int ve, unsigned char *nullspr)
{
	p = cadd(p, 0x1FC, 0x0001);                    /* FMODE: 32-bit bitplane fetch */
	p = cadd(p, 0x100, 0x0211);                    /* BPLCON0: 8 planes, colour, ECSENA */
	p = cadd(p, 0x102, 0); p = cadd(p, 0x104, 0x0024); p = cadd(p, 0x10C, 0x0011);
	p = cadd(p, 0x08E, (vs << 8) | 0x81); p = cadd(p, 0x090, ((ve & 255) << 8) | 0xC1); p = cadd(p, 0x1E4, 0x2000 | ((ve > 255) ? 0x100 : 0));
	p = cadd(p, 0x092, 0x0030); p = cadd(p, 0x094, 0x00C0);
	p = cadd(p, 0x108, 280); p = cadd(p, 0x10A, 280);
	for (int i = 0; i < 8; i++) { unsigned a = (unsigned)planes + i * 40; p = cadd(p, 0xE0 + i * 4, a >> 16); p = cadd(p, 0xE2 + i * 4, a & 0xffff); }
	for (int i = 0; i < 8; i++) { unsigned a = (unsigned)nullspr; p = cadd(p, 0x120 + i * 4, a >> 16); p = cadd(p, 0x122 + i * 4, a & 0xffff); }
	return p;
}
void plat_video_open(void)
{
	if (active) return;
	IntuitionBase = OpenLibrary("intuition.library", 39);
	GfxBase = OpenLibrary("graphics.library", 40);
	/* keyboard window on the Workbench screen */
	NewWindow nw = { 0, 0, 64, 32, 0, 1, 0x400, 0x800 | 0x1000 | 0x10000 | 0x4000 /*BORDERLESS ACTIVATE RMBTRAP NOCAREREFRESH*/, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 /*WBENCHSCREEN*/ };
	window = OpenWindow(&nw);
	chip_ptr = AllocMem(16, MEMF_CHIP | MEMF_CLEAR);
	if (window && chip_ptr) SetPointer(window, chip_ptr, 1, 1, 0, 0);
	planes = AllocMem(320 * 200 + 64, MEMF_CHIP | MEMF_CLEAR);
	cop = AllocMem(512, MEMF_CHIP | MEMF_CLEAR);
	unsigned char *nullspr = AllocMem(16, MEMF_CHIP | MEMF_CLEAR);
	int pal = ((unsigned char *)GfxBase)[206] & 4;
	int vs = pal ? 0x48 : 0x2C, ve = vs + 200;
	cop_pal = AllocMem(4096, MEMF_CHIP | MEMF_CLEAR);
	g_vs = vs; g_ve = ve; g_nullspr = nullspr;
	unsigned short *p = build_base(cop, vs, ve, nullspr);
	*p++ = 0xFFFF; *p++ = 0xFFFE;
	oldview = *(void **)((char *)GfxBase + 34);
	LoadView(0); WaitTOF(); WaitTOF();
	R16(0x096) = 0x0020;                           /* sprite DMA off */
	R16(0x106) = 0x0C20;                           /* BPLCON3 once (the Copper no longer writes it every frame) */
	R16(0x080) = (unsigned)cop >> 16; R16(0x082) = (unsigned)cop & 0xffff; R16(0x088) = 0;
	R16(0x096) = 0x8000 | 0x0200 | 0x0100 | 0x0080;   /* DMAEN BPLEN COPEN */
	active = 1; pal_dirty = 1;
	inject_init();
	{ extern char isr_ring[]; char b[40]; sprintf(b, "ISRRING %x\n", (unsigned)isr_ring); dbg_write(b); }
}
void plat_video_close(void)
{
	if (!active) return;
	active = 0;
	LoadView(oldview); WaitTOF(); WaitTOF();
	if (window) { CloseWindow(window); window = 0; }
}
void dbg_flush(void);
char dump_name[24]; volatile int dump_req;
extern unsigned char dac[768];
static void do_dump(void)
{
	char path[40]; int i = 0, k = 0; const char *pre = "SYS:";
	while (pre[k]) path[i++] = pre[k++];
	for (k = 0; dump_name[k]; k++) path[i++] = dump_name[k];
	path[i++] = '.'; path[i++] = 'r'; path[i++] = 'a'; path[i++] = 'w'; path[i] = 0;
	LONG fh = Open(path, MODE_NEWFILE);
	if (fh) { Write(fh, plat_vram, 64000); Write(fh, dac, 768); Close(fh); }
	dump_req = 0;
}
void plat_idle(void) { dbg_flush(); if (dump_req) do_dump(); }
void plat_present(void)
{
	dbg_flush();
	if (dump_req) do_dump();
#ifdef FPS_TRACE
	{ static unsigned n; extern volatile unsigned plat_vbl_count; if (++n % 100 == 0) { char b[40]; sprintf(b, "PRES %u\n", n); dbg_write(b); } }
#endif
	if (!active) return;
#ifdef NOC2P
	return;
#endif
	const unsigned char *src = plat_vram; unsigned char *dst = planes;
	static unsigned shadow[320 * 200 / 4];
	c2p_frame(plat_vram, shadow, planes);
}
/* Convert only the 16-pixel groups that cover the rectangle (used for the mouse cursor, which the game draws straight into the screen
   buffer from a timer service: waiting for the next full-screen update made the pointer lag badly on static screens). */
void plat_present_rect(int x, int y, int w, int h)
{
	if (!active) return;
	if (x < 0) { w += x; x = 0; } if (y < 0) { h += y; y = 0; }
	if (x + w > 320) w = 320 - x; if (y + h > 200) h = 200 - y;
	if (w <= 0 || h <= 0) return;
	int g0 = x >> 4, g1 = (x + w - 1) >> 4;
	for (int yy = y; yy < y + h; yy++)
		for (int g = g0; g <= g1; g++) c2p_pair(plat_vram + yy * 320 + g * 16, planes + yy * 320 + g * 2);
}
void plat_mouse_set(int x, int y) { mouse_x = x; mouse_y = y; }

/* Amiga raw key -> PC set-1 scan code */
static const unsigned char rawmap[0x68] = {
 /*00*/ 0x29,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x2b,0,0x52,
 /*10*/ 0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1a,0x1b,0,0x4f,0x50,0x51,
 /*20*/ 0x1e,0x1f,0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0,0,0x4b,0x4c,0x4d,
 /*30*/ 0x56,0x2c,0x2d,0x2e,0x2f,0x30,0x31,0x32,0x33,0x34,0x35,0,0x53,0x47,0x48,0x49,
 /*40*/ 0x39,0x0e,0x0f,0x1c,0x1c,0x01,0x53,0,0,0,0x4a,0,0x48,0x50,0x4d,0x4b,
 /*50*/ 0x3b,0x3c,0x3d,0x3e,0x3f,0x40,0x41,0x42,0x43,0x44,0,0,0,0,0,0x3b,
 /*60*/ 0x2a,0x36,0x3a,0x1d,0x38,0x38,0,0 };
static void key_event(int code)
{
	int up = code & 0x80; code &= 0x7f;
	if (code >= 0x68) return;
	int sc = rawmap[code]; if (!sc) return;
	KBD_ReadScan(up ? (sc | 0x80) : sc);
}
void plat_poll_input(void)
{
	if (!window) return;
	void *port = *(void **)((char *)window + 86);
	unsigned char *m;
	while ((m = GetMsg(port))) {
		ULONG cls = *(ULONG *)(m + 20); unsigned code = *(unsigned short *)(m + 24);
		ReplyMsg(m);
		if (cls == 0x400) key_event(code);
	}
}
