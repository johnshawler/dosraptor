/* Emulation of the Watcom/DOS4GW API surface the game uses (see plat/dosemu.c). */
#ifndef SHIM_DOS_H
#define SHIM_DOS_H
#include <stdlib.h>
#include <string.h>
union REGS {
	struct { unsigned int eax, ebx, ecx, edx, esi, edi, cflag; } x;
	struct { unsigned short _ah, ax, _bh, bx, _ch, cx, _dh, dx, _sh, si, _dih, di; } w;
	struct { unsigned char _a0, _a1, ah, al, _b0, _b1, bh, bl, _c0, _c1, ch, cl, _d0, _d1, dh, dl; } h;
};
struct SREGS { unsigned short es, cs, ss, ds, fs, gs; };
struct dosdate_t { unsigned char day, month; unsigned short year; unsigned char dayofweek; };
int  int386(int n, const union REGS *in, union REGS *out);
int  int386x(int n, const union REGS *in, union REGS *out, struct SREGS *s);
void segread(struct SREGS *s);
void _dos_getdate(struct dosdate_t *d);
int  inp(int port);
int  outp(int port, int v);
void _enable(void);
void _disable(void);
#define FP_OFF(p) ((unsigned)(p))
#define FP_SEG(p) 0
#define _loadds
#define far
#define _far
#define cdecl
#define F_OK 0
#endif
#define _dos_getvect(x) ((void*)0)
#define _dos_setvect(x,y) ((void)0)
