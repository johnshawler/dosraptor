#include "os.h"
void *DOSBase;
int amiga_main(void)
{
	DOSBase = OpenLibrary("dos.library", 36);
	if (!DOSBase) return 20;
	LONG fh = Open("SYS:hello.txt", MODE_NEWFILE);
	if (fh) { Write(fh, "hello from 68k\n", 15); Close(fh); }
	CloseLibrary(DOSBase);
	return 0;
}
