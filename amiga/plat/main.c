#include "os.h"
#include "plat.h"
extern void *DOSBase;
extern void raptor_main(int argc, char **argv);
int amiga_main(void)
{
	static char *argv[] = { "RAPTOR", 0, 0 };
	DOSBase = OpenLibrary("dos.library", 36);
	if (!DOSBase) return 20;
	raptor_main(1, argv);
	return 0;
}
