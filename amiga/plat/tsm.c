/* Timer services (TSM_*) driven by the vertical blank interrupt. */
#include "os.h"
#include "plat.h"
#define MAXSVC 16
typedef struct { void (*fn)(void); int rate; int pause; int used; int acc; int pri; } Svc;
static Svc svc[MAXSVC];
volatile unsigned plat_vbl_count;
extern void inject_tick(void);
extern void plat_mouse_resend(void);
int plat_vbl_hz = 50, plat_pal = 1;
#define vbl_hz plat_vbl_hz
static struct __attribute__((packed)) { void *succ, *pred; unsigned char type, pri; char *name; void *data; void *code; } is;
extern void vbl_isr_stub(void);
static int installed;
static int base_hz = 140, cia_ok;            /* service base rate; CIA timer interrupt present */
static void run_services(int hz)
{
	for (int i = 0; i < MAXSVC; i++) {
		if (!svc[i].used || svc[i].pause) continue;
		svc[i].acc += svc[i].rate;
		while (svc[i].acc >= hz) { svc[i].acc -= hz; svc[i].fn(); }
	}
}
void tsm_vbl(void)
{
	plat_vbl_count++;
	plat_poll_input();
	plat_vbl_hw();
	if (!cia_ok) plat_mouse_poll();
	inject_tick();
	plat_mouse_resend();
	if (!cia_ok) run_services(vbl_hz);
}
extern void plat_mouse_poll(void);
void tsm_cia(void) { plat_mouse_poll(); run_services(base_hz); }

/* CIA-B timer interrupt through cia.resource (the OS owns the CIAs, so go through AddICRVector) */
static struct __attribute__((packed)) { void *succ, *pred; unsigned char type, pri; char *name; void *data; void *code; } cis;
extern void cia_isr_stub(void);
#define CIAB(r) (*(volatile unsigned char *)(0xBFD000 + (r) * 0x100))
static void *cia_res;
static int cia_bit;
static void *OpenResource(const char *n)
{
	register const char *_n __asm("a1") = n; register void *_r __asm("d0");
	__asm volatile("move.l %%a6,-(%%sp)\n\tmove.l %[b],%%a6\n\tjsr -498(%%a6)\n\tmove.l (%%sp)+,%%a6" : "=r"(_r), "+r"(_n) : [b]"a"(SysBase) : "d1", "a0", "memory", "cc");
	return _r;
}
static int AddICRVector(void *res, int bit, void *intr)
{
	register int _b __asm("d0") = bit; register void *_i __asm("a1") = intr;
	__asm volatile("move.l %%a6,-(%%sp)\n\tmove.l %[b],%%a6\n\tjsr -6(%%a6)\n\tmove.l (%%sp)+,%%a6" : "+r"(_b), "+r"(_i) : [b]"a"(res) : "d1", "a0", "memory", "cc");
	return _b;
}
static void AbleICR(void *res, int mask)
{
	register int _m __asm("d0") = mask;
	__asm volatile("move.l %%a6,-(%%sp)\n\tmove.l %[b],%%a6\n\tjsr -18(%%a6)\n\tmove.l (%%sp)+,%%a6" : "+r"(_m) : [b]"a"(res) : "d1", "a0", "a1", "memory", "cc");
}
static void cia_start(int hz)
{
	cia_res = OpenResource("ciab.resource");
	if (!cia_res) return;
	cis.type = 2; cis.pri = 0; cis.name = "raptor.tick"; cis.data = 0; cis.code = (void *)cia_isr_stub;
	for (cia_bit = 0; cia_bit < 2; cia_bit++) if (AddICRVector(cia_res, cia_bit, &cis) == 0) break;
	if (cia_bit == 2) return;
	unsigned clock = plat_pal ? 709379u : 715909u, latch = clock / hz - 1;
	base_hz = hz;
	if (cia_bit == 0) { CIAB(14) = 0; CIAB(4) = latch & 255; CIAB(5) = latch >> 8; }
	else { CIAB(15) = 0; CIAB(6) = latch & 255; CIAB(7) = latch >> 8; }
	cia_ok = 1;
	AbleICR(cia_res, 0x80 | (1 << cia_bit));
	if (cia_bit == 0) CIAB(14) = 0x11; else CIAB(15) = 0x11;   /* load + start, continuous */
}
void TSM_Install(int rate)
{
	if (installed) return;
	/* PAL flag: GfxBase->DisplayFlags (offset 206), PAL = 4 */
	void *gb = OpenLibrary("graphics.library", 36);
	if (gb) { unsigned char fl = *(unsigned char *)((char *)gb + 206); vbl_hz = (fl & 4) ? 50 : 60; plat_pal = (fl & 4) ? 1 : 0; CloseLibrary(gb); }
	is.type = 2 /*NT_INTERRUPT*/; is.pri = 0; is.name = "raptor.vbl"; is.data = 0; is.code = (void *)vbl_isr_stub;
	AddIntServer(5 /*INTB_VERTB*/, &is);
	installed = 1;
	cia_start(rate);
}
void TSM_Remove(void) { if (installed) { RemIntServer(5, &is); installed = 0; } }
int TSM_NewService(void (*function)(void), int rate, int priority, int pause)
{
	for (int i = 0; i < MAXSVC; i++) if (!svc[i].used) { svc[i].fn = function; svc[i].rate = rate; svc[i].pri = priority; svc[i].pause = pause; svc[i].acc = 0; svc[i].used = 1; return i; }
	return -1;
}
void TSM_DelService(int id) { if (id >= 0 && id < MAXSVC) svc[id].used = 0; }
void TSM_PauseService(int id) { if (id >= 0 && id < MAXSVC) svc[id].pause = 1; }
void TSM_ResumeService(int id) { if (id >= 0 && id < MAXSVC) svc[id].pause = 0; }
