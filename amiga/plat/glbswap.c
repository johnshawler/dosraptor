/* Convert GLB items from the little-endian PC layout to native (big endian) byte order when they are loaded.
   Which layout an item has is derived from the suffix of its directory name (X_PIC, X_FNT, ...). */
typedef unsigned char BYTE; typedef unsigned int DWORD; typedef unsigned short WORD;
static int strcmp3(const char *a, const char *b) { return a[0] != b[0] || a[1] != b[1] || a[2] != b[2] || a[3]; }

static inline DWORD ld32(const BYTE *p) { return p[0] | (p[1] << 8) | (p[2] << 16) | ((DWORD)p[3] << 24); }
static inline void sw32(BYTE *p) { BYTE a = p[0], b = p[1]; p[0] = p[3]; p[1] = p[2]; p[2] = b; p[3] = a; }
static inline void sw16(BYTE *p) { BYTE a = p[0]; p[0] = p[1]; p[1] = a; }
static void swn32(BYTE *p, int n) { while (n--) { sw32(p); p += 4; } }

static void swap_pic(BYTE *p, unsigned size)
{
	if (size < 20) return;
	swn32(p, 5);
	DWORD type = *(DWORD *)p;
	if (type != 0) return;               /* GPIC: raw pixels follow the header */
	unsigned pos = 20;
	while (pos + 16 <= size) {
		BYTE *s = p + pos; swn32(s, 4);
		unsigned off = ((DWORD *)s)[2] & 0xffff, len = ((DWORD *)s)[3] & 0xffff;
		if (off == 0xffff) break;
		pos += 16 + len;
	}
}
static void swap_fnt(BYTE *p, unsigned size)
{
	if (size < 4 + 512) return;
	sw32(p); for (int i = 0; i < 256; i++) sw16(p + 4 + i * 2);
}
static void swap_swd(BYTE *p, unsigned size)
{
	if (size < 120) return;
	swn32(p, 8); swn32(p + 8 * 4 + 32, 14);
	DWORD fldofs = *(DWORD *)(p + 8 * 4 + 32 + 3 * 4), numflds = *(DWORD *)(p + 8 * 4 + 32 + 8 * 4);
	for (DWORD i = 0; i < numflds; i++) {
		BYTE *f = p + fldofs + i * 144; if (f + 144 > p + size) break;
		swn32(f, 8); swn32(f + 32 + 32, 1); swn32(f + 32 + 32 + 4 + 16, 15);
	}
}

void GLB_SwapItem(const char *rawname, BYTE *p, unsigned size)
{
	char name[17]; int i;
	for (i = 0; i < 16 && rawname[i]; i++) name[i] = rawname[i];
	name[i] = 0;
	const char *suf = 0;
	for (int k = 0; k < i; k++) if (name[k] == '_') suf = name + k + 1;
	if (suf && (!strcmp3(suf, "PIC") || !strcmp3(suf, "BLK"))) { swap_pic(p, size); return; }
	if (suf && !strcmp3(suf, "FNT")) { swap_fnt(p, size); return; }
	if (suf && !strcmp3(suf, "SWD")) { swap_swd(p, size); return; }
	if (!suf && size >= 20) {            /* unnamed run (e.g. tile pictures): detect GFX_PIC by header */
		DWORD t = ld32(p), w = ld32(p + 12), h = ld32(p + 16);
		if ((t == 0 || t == 1) && w > 0 && w <= 320 && h > 0 && h <= 200) swap_pic(p, size);
	}
}
