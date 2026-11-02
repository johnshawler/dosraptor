/* Timer services (TSM_*) driven by the vertical blank interrupt. */
#include "os.h"
#include "plat.h"
#define MAXSVC 16
typedef struct { void (*fn)(void); int rate; int pause; int used; int acc; int pri; } Svc;
static Svc svc[MAXSVC];
volatile unsigned plat_vbl_count;
static int vbl_hz = 50;
static struct __attribute__((packed)) { void *succ, *pred; unsigned char type, pri; char *name; void *data; void *code; } is;
extern void vbl_isr_stub(void);
static int installed;
void tsm_vbl(void)
{
	plat_vbl_count++;
	plat_poll_input();
	plat_vbl_hw();
	for (int i = 0; i < MAXSVC; i++) {
		if (!svc[i].used || svc[i].pause) continue;
		svc[i].acc += svc[i].rate;
		while (svc[i].acc >= vbl_hz) { svc[i].acc -= vbl_hz; svc[i].fn(); }
	}
}
void TSM_Install(int rate)
{
	if (installed) return;
	/* PAL flag: GfxBase->DisplayFlags (offset 206), PAL = 4 */
	void *gb = OpenLibrary("graphics.library", 36);
	if (gb) { unsigned char fl = *(unsigned char *)((char *)gb + 206); vbl_hz = (fl & 4) ? 50 : 60; CloseLibrary(gb); }
	is.type = 2 /*NT_INTERRUPT*/; is.pri = 0; is.name = "raptor.vbl"; is.data = 0; is.code = (void *)vbl_isr_stub;
	AddIntServer(5 /*INTB_VERTB*/, &is);
	installed = 1;
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
