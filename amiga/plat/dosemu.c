/* DOS/BIOS/VGA/mouse API emulation on top of the Amiga platform layer. */
#include "os.h"
#include "plat.h"
#include "../shim/dos.h"

/* ---- VGA DAC ---- */
unsigned char dac[768];
static int dac_w, dac_r, dac_wphase, dac_rphase;
static unsigned retrace_last;
static void (*mouse_handler)(int bx, int cx, int dx);
static unsigned mouse_mask;
int outp(int port, int v)
{
	v &= 0xff;
	switch (port) {
	case 0x3C8: dac_w = v; dac_wphase = 0; break;
	case 0x3C7: dac_r = v; dac_rphase = 0; break;
	case 0x3C9:
		dac[dac_w * 3 + dac_wphase] = v;
		if (++dac_wphase == 3) { plat_set_color(dac_w, dac[dac_w*3], dac[dac_w*3+1], dac[dac_w*3+2]); dac_wphase = 0; dac_w = (dac_w + 1) & 255; }
		break;
	}
	return v;
}
int inp(int port)
{
	switch (port) {
	case 0x3C9: { int v = dac[dac_r * 3 + dac_rphase]; if (++dac_rphase == 3) { dac_rphase = 0; dac_r = (dac_r + 1) & 255; } return v; }
	case 0x3DA: { unsigned c = plat_vbl_count; if (c != retrace_last) { retrace_last = c; return 8; } return 0; }
	case 0x201: return 0x0f;   /* no analog joystick: axes never time out -> PTR_IsJoyPresent false */
	}
	return 0;
}
void _enable(void) {}
void _disable(void) {}
void segread(struct SREGS *s) { s->es = s->cs = s->ss = s->ds = s->fs = s->gs = 0; }
void _dos_getdate(struct dosdate_t *d) { d->day = 1; d->month = 6; d->year = 1996; d->dayofweek = 0; }

int int386x(int n, const union REGS *in, union REGS *out, struct SREGS *s)
{
	if (out != in) *out = *in;
	switch (n) {
	case 0x10:
		if (in->w.ax == 0x13) plat_video_open();
		else if (in->w.ax == 0x03) plat_video_close();
		break;
	case 0x33:
		switch (in->w.ax) {
		case 0: out->w.ax = -1; out->w.bx = 2; break;
		case 4: plat_mouse_set(in->x.ecx >> 1, in->x.edx); break;
		case 0x0C: mouse_mask = in->w.cx; mouse_handler = (void (*)(int,int,int))in->x.edx; break;
		}
		break;
	}
	return 0;
}
int int386(int n, const union REGS *in, union REGS *out) { return int386x(n, in, out, 0); }
/* The game's mouse handler ignores events that arrive while it redraws the cursor.  Real drivers deliver them again on the next
   movement; here every change is repeated on the following vertical blanks so that a dropped press/release cannot get lost. */
#ifndef RESEND
#define RESEND 6
#endif
static int ev_b, ev_x, ev_y, ev_repeat;
void plat_mouse_event(int buttons, int x, int y) { ev_b = buttons; ev_x = x; ev_y = y; ev_repeat = RESEND; if (mouse_handler) mouse_handler(buttons, x << 1, y); }
void plat_mouse_resend(void) { if (ev_repeat > 0 && mouse_handler) { ev_repeat--; mouse_handler(ev_b, ev_x << 1, ev_y); } }

/* ---- DPMI memory ---- */
int _dpmi_dosalloc(unsigned short paras, unsigned int *segment)
{
	if (paras > 4096) return 8;            /* no "low memory" pool: the 8 MB of fast RAM go to the game's VM heap instead */
	void *p = AllocMem((unsigned)paras * 16 + 16, MEMF_CLEAR);
	if (!p) return 8;
	*segment = (((unsigned)p + 15) & ~15u) >> 4;
	return 0;
}
int _dpmi_lockregion(void *a, unsigned l) { return 0; }
int _dpmi_unlockregion(void *a, unsigned l) { return 0; }
int _dpmi_getmemsize(void)
{
	unsigned avail = AvailMem(MEMF_FAST | MEMF_LARGEST);
	if (!avail) avail = AvailMem(MEMF_ANY | MEMF_LARGEST);
	return avail > 0x780000 ? 0x780000 : avail;
}
