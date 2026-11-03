/* C replacements for the 386 assembler routines (GFXAPI_A.ASM, TILE_A.ASM, MOVIE_A.ASM, PTRAPI_A.ASM).
   Data from the GLB files is converted to native (big endian) byte order when loaded (see glbswap.c). */
#include "os.h"
#include <string.h>
void plat_present(void);
typedef unsigned char BYTE; typedef int INT; typedef unsigned int DWORD; typedef unsigned short UW;

#define SCREENWIDTH 320
extern BYTE *displaybuffer, *displayscreen;
extern INT stable[], tablelen, ud_x, ud_y, ud_lx, ud_ly, update_start;
extern BYTE *gfx_inmem; extern INT gfx_xp, gfx_yp, gfx_lx, gfx_ly, gfx_imga;
extern BYTE *tilepic, *tilestart; extern INT tileloopy, g_mapleft;
extern BYTE *cursorstart, *displaypic, *cursorsave; extern INT cursorloopx, cursorloopy, joy_x, joy_y, joy_buttons;

typedef struct { INT x, y, offset, length; } SPR;

void GFX_ScaleLine(BYTE *out, BYTE *in)
{
	INT n = tablelen; const INT *t = stable + n - 1;
	for (INT i = 0; i < n; i++) out[i] = in[*t--];
}
void GFX_CScaleLine(BYTE *out, BYTE *in)
{
	INT n = tablelen; const INT *t = stable + n - 1;
	for (INT i = 0; i < n; i++) { BYTE b = in[*t--]; if (b) out[i] = b; }
}
void GFX_ShadeSprite(BYTE *dest, BYTE *inmem, BYTE *dtable)
{
	BYTE *tb = (BYTE *)((DWORD)dtable & ~255u);
	for (;;) {
		SPR *s = (SPR *)inmem; UW off = (UW)s->offset;
		if (off == 0xffff) return;
		UW len = (UW)s->length; BYTE *p = dest + off;
		inmem += sizeof(SPR) + len;
		while (len--) { *p = tb[*p]; p++; }
	}
}
void GFX_DrawSprite(BYTE *dest, BYTE *inmem)
{
	for (;;) {
		SPR *s = (SPR *)inmem; UW off = (UW)s->offset;
		if (off == 0xffff) return;
		UW len = (UW)s->length;
		memcpy(dest + off, inmem + sizeof(SPR), len);
		inmem += sizeof(SPR) + len;
	}
}
void GFX_DrawChar(BYTE *dest, BYTE *inmem, INT lx, INT ly, INT addx, INT color)
{
	BYTE c = (BYTE)color;
	while (ly--) {
		for (INT i = 0; i < lx; i++) { BYTE b = inmem[i]; if (b) dest[i] = b + c; }
		inmem += lx + addx; dest += SCREENWIDTH;
	}
}
void GFX_Shade(BYTE *out, INT maxlen, BYTE *dtable)
{
	BYTE *tb = (BYTE *)((DWORD)dtable & ~255u);
	while (maxlen-- > 0) { *out = tb[*out]; out++; }
}
void GFX_PutPic(void)
{
	BYTE *src = gfx_inmem, *dst = displaybuffer + gfx_xp + gfx_yp * SCREENWIDTH;
	INT lx = gfx_lx, ly = gfx_ly;
	while (ly--) { memcpy(dst, src, lx); src += lx + gfx_imga; dst += SCREENWIDTH; }
}
void GFX_PutMaskPic(void)
{
	BYTE *src = gfx_inmem, *dst = displaybuffer + gfx_xp + gfx_yp * SCREENWIDTH;
	INT lx = gfx_lx, ly = gfx_ly;
	if (lx <= 0) return;
	while (ly--) {
		for (INT i = 0; i < lx; i++) if (src[i]) dst[i] = src[i];
		src += gfx_imga; dst += SCREENWIDTH;
	}
}
void GFX_DisplayScreen(void)
{
	BYTE *s = displaybuffer + ud_x + ud_y * SCREENWIDTH, *d = displayscreen + ud_x + ud_y * SCREENWIDTH;
	if (ud_lx == SCREENWIDTH) memcpy(d, s, (ud_lx * ud_ly) & ~3);
	else { INT n = (ud_lx >> 2) << 2, ly = ud_ly; while (ly--) { memcpy(d, s, n); s += SCREENWIDTH; d += SCREENWIDTH; } }
	update_start = 0;
	plat_present();
}

/* ---- tiles ---- */
void TILE_Draw(void)
{
	BYTE *s = tilepic, *d = tilestart;
	for (int y = 0; y < 32; y++) { memcpy(d, s, 32); s += 32; d += SCREENWIDTH; }
}
void TILE_ClipDraw(void)
{
	BYTE *s = tilepic, *d = tilestart;
	for (int y = 0; y < tileloopy; y++) { memcpy(d, s, 32); s += 32; d += SCREENWIDTH; }
}
void TILE_ShakeScreen(void)
{
	BYTE *s = displaybuffer + 16 - 4, *d = displayscreen + g_mapleft - 4;
	for (int y = 0; y < 200; y++) { memcpy(d, s, 296); s += SCREENWIDTH; d += SCREENWIDTH; }
	plat_present();
}
void TILE_DisplayScreen(void)
{
	BYTE *s = displaybuffer + 16, *d = displayscreen + 16;
	for (int y = 0; y < 200; y++) { memcpy(d, s, 288); s += SCREENWIDTH; d += SCREENWIDTH; }
	plat_present();
}

/* ---- movie ---- */
typedef struct { UW opt, fill, offset, length; } ANIML;
void ANIM_Render(BYTE *inmem)
{
	for (;;) {
		ANIML *a = (ANIML *)inmem;
		if (a->opt == 0) return;
		memcpy(displaybuffer + a->offset, inmem + sizeof(ANIML), a->length);
		inmem += sizeof(ANIML) + a->length;
	}
}

/* ---- pointer ---- */
void PTR_Save(void) { BYTE *s = cursorstart, *d = cursorsave; for (int y = 0; y < 16; y++) { memcpy(d, s, 16); d += 16; s += SCREENWIDTH; } }
void PTR_ClipSave(void) { BYTE *s = cursorstart, *d = cursorsave; for (int y = 0; y < cursorloopy; y++) { memcpy(d, s, 16); d += 16; s += SCREENWIDTH; } }
void PTR_Erase(void) { BYTE *s = cursorsave, *d = cursorstart; for (int y = 0; y < 16; y++) { memcpy(d, s, 16); s += 16; d += SCREENWIDTH; } }
void PTR_ClipErase(void) { BYTE *s = cursorsave, *d = cursorstart; for (int y = 0; y < cursorloopy; y++) { memcpy(d, s, cursorloopx); s += 16; d += SCREENWIDTH; } }
void PTR_Draw(void)
{
	BYTE *s = displaypic, *d = cursorstart;
	for (int y = 0; y < cursorloopy; y++) {
		for (int x = 0; x < cursorloopx; x++) if (s[x]) d[x] = s[x];
		s += 16; d += SCREENWIDTH;
	}
}
void PTR_ReadJoyStick(void)
{
	volatile unsigned short *joy1dat = (volatile unsigned short *)0xDFF00C;
	volatile unsigned char *ciaa = (volatile unsigned char *)0xBFE001;
	volatile unsigned short *potgor = (volatile unsigned short *)0xDFF016;
	unsigned j = *joy1dat;
	int right = (j >> 1) & 1, left = (j >> 9) & 1, down = ((j >> 1) ^ j) & 1, up = ((j >> 9) ^ (j >> 8)) & 1;
	joy_x = right ? 40 : left ? -40 : 0; joy_y = down ? 40 : up ? -40 : 0;
	joy_buttons = ((*ciaa & 0x80) ? 0 : 1) | ((*potgor & 0x4000) ? 0 : 2);
}
