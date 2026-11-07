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
		BYTE *f = p + fldofs + i * 148; if (f + 148 > p + size) break;
		swn32(f, 8); swn32(f + 32 + 32, 1); swn32(f + 32 + 32 + 4 + 16, 16);
	}
}

/* AGX: stream of ANIMLINE records {opt,fill,offset,length: u16 each} + length data bytes, ended by a record with opt == 0.
   The first frame of a movie has one extra leading fill byte; the layout that ends exactly at the item size is the right one. */
static int agx_walk(const BYTE *p, unsigned size, unsigned start)
{
	unsigned pos = start;
	for (;;) {
		if (pos + 8 > size) return -1;
		unsigned opt = p[pos] | (p[pos + 1] << 8);
		if (opt == 0) return (int)(pos + 8);
		pos += 8 + (p[pos + 6] | (p[pos + 7] << 8));
	}
}
static void swap_agx(BYTE *p, unsigned size)
{
	unsigned start = 0;
	if (agx_walk(p, size, 0) != (int)size && agx_walk(p, size, 1) == (int)size) start = 1;
	if (agx_walk(p, size, start) < 0) return;
	unsigned pos = start;
	for (;;) {
		unsigned opt = p[pos] | (p[pos + 1] << 8), len = p[pos + 6] | (p[pos + 7] << 8);
		sw16(p + pos); sw16(p + pos + 2); sw16(p + pos + 4); sw16(p + pos + 6);
		if (opt == 0) break;
		pos += 8 + len;
	}
}

static void swap_map(BYTE *p, unsigned size)
{
	/* MAZELEVEL: 3 ints, 1350 x {short flats, short fgame}; then numsprites x CSPRITE (6 ints) */
	if (size < 12 + 1350 * 4) return;
	swn32(p, 3);
	unsigned n = *(DWORD *)(p + 8);
	for (int i = 0; i < 2700; i++) sw16(p + 12 + i * 2);
	BYTE *c = p + 12 + 1350 * 4;
	for (unsigned i = 0; i < n && c + 24 <= p + size; i++, c += 24) swn32(c, 6);
}
static void swap_itm(const char *name, BYTE *p, unsigned size)
{
	if (name[0] == 'F') {                 /* FLATS: {int, short, short} */
		for (unsigned o = 0; o + 8 <= size; o += 8) { sw32(p + o); sw16(p + o + 4); sw16(p + o + 6); }
	} else {                              /* SPRITE: char[16], 26 ints, 204 shorts = 528 bytes */
		for (unsigned o = 0; o + 528 <= size; o += 528) { swn32(p + o + 16, 26); for (int i = 0; i < 204; i++) sw16(p + o + 120 + i * 2); }
	}
}

static void swap_rec(BYTE *p, unsigned size)
{
	/* demo recording: RECORD {BYTE b1..b4; SHORT px, py, playerpic, fil} = 12 bytes */
	for (unsigned o = 0; o + 12 <= size; o += 12) { sw16(p + o + 4); sw16(p + o + 6); sw16(p + o + 8); sw16(p + o + 10); }
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
	if (suf && !strcmp3(suf, "REC")) { swap_rec(p, size); return; }
	if (suf && !strcmp3(suf, "MAP")) { swap_map(p, size); return; }
	if (suf && !strcmp3(suf, "ITM")) { swap_itm(name, p, size); return; }
	if (suf && !strcmp3(suf, "AGX")) { swap_agx(p, size); return; }
	if (suf && !strcmp3(suf, "SWD")) { swap_swd(p, size); return; }
	if (!suf && size >= 20) {            /* unnamed run (e.g. tile pictures): detect GFX_PIC by header */
		DWORD t = ld32(p), w = ld32(p + 12), h = ld32(p + 16);
		if ((t == 0 || t == 1) && w > 0 && w <= 320 && h > 0 && h <= 200) swap_pic(p, size);
	}
}
