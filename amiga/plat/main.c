#include "os.h"
#include "plat.h"
extern void *DOSBase;
extern void raptor_main(int argc, char **argv);
int amiga_main(void)
{
	static char *argv[] = { "RAPTOR", "", "", "" };   /* never NULL: the game compares argv[1] unconditionally */
	CacheControl(0x0011, 0x0011);          /* 68030: instruction cache + burst (data cache stays as the OS set it: custom registers are read) */
	DOSBase = OpenLibrary("dos.library", 36);
	if (!DOSBase) return 20;
	raptor_main(1, argv);
	return 0;
}
