/* Audio: software mixer on Paula channels 0 (left) and 1 (right).
   Voices: DMX digital sound effects (8 voices, 8-bit unsigned PCM, any rate/pitch) and one pre-rendered music stream (signed 8-bit,
   16 kHz, rendered offline from the OPL2/GENMIDI score by tools/mus2pcm.py).  Paula plays two looping chip-RAM buffers per channel;
   the AUD0 interrupt (start of every pass) mixes the next block into the buffer that has just finished. */
#include "os.h"
#include "plat.h"
#define R16(o) (*(volatile unsigned short *)(0xDFF000 + (o)))
#define AUD(c, r) (0x0A0 + (c) * 0x10 + (r))
#define NS 512                       /* samples per block (32 ms) */
#define NV 8
#define MIXRATE_PAL 3546895u
extern volatile unsigned plat_vbl_count;
extern int plat_pal, plat_vbl_hz;
void isr_log(const char *f, ...);
void *memcpy(void *, const void *, unsigned);
void *malloc(unsigned); void free(void *);
int open(const char *, int, ...); int read(int, void *, unsigned); int close(int);
long filelength(int);
int sprintf(char *, const char *, ...);

static signed char *blk[2][2];       /* [buffer][channel] chip RAM */
static int play, started, mixrate;
typedef struct { const signed char *d; unsigned len, pos, step; int gl, gr, id; unsigned long long end; int active; } Voice;  /* d: signed samples */
static Voice vox[NV];
static signed char *vbuf[NV];        /* private signed copies of the effect samples */
#define VBUFSZ 49152
static int next_id = 1;

/* music */
static struct { signed char *d; unsigned len, pos; int loop, active, vol, fadeto, fadems_left, fadestep, id, loaded; char name[24]; unsigned crc; } mus;
static int music_master = 127;
static struct { unsigned len, crc; char name[24]; } idx[24]; static int nidx, idx_loaded;

static unsigned crc32(const unsigned char *p, unsigned n)
{
	unsigned c = 0xffffffffu;
	while (n--) { c ^= *p++; for (int k = 0; k < 8; k++) c = (c >> 1) ^ (0xEDB88320u & -(c & 1)); }
	return ~c;
}
static void load_index(void)
{
	if (idx_loaded) return; idx_loaded = 1;
	int fd = open("SYS:MUSIC/INDEX.TXT", 0); if (fd < 0) return;
	char b[1024]; int n = read(fd, b, 1023); close(fd); if (n <= 0) return; b[n] = 0;
	char *p = b;
	while (*p && nidx < 24) {
		unsigned len = 0, crc = 0; while (*p == ' ') p++;
		while (*p >= '0' && *p <= '9') len = len * 10 + *p++ - '0';
		while (*p == ' ') p++;
		for (;; p++) { int d = (*p >= '0' && *p <= '9') ? *p - '0' : (*p >= 'a' && *p <= 'f') ? *p - 'a' + 10 : -1; if (d < 0) break; crc = crc * 16 + d; }
		while (*p == ' ') p++;
		int i = 0; while (*p && *p != '\n' && i < 23) idx[nidx].name[i++] = *p++; idx[nidx].name[i] = 0;
		while (*p && *p != '\n') p++; if (*p) p++;
		idx[nidx].len = len; idx[nidx].crc = crc; nidx++;
	}
}

/* ---- mixer interrupt ---- */
void mix_block(void)
{
	static int mixed[NS * 2];
	int fill = 1 - play;
	for (int i = 0; i < NS * 2; i++) mixed[i] = 0;
	for (int v = 0; v < NV; v++) {
		Voice *x = &vox[v]; if (!x->active) continue;
		const signed char *d = x->d; unsigned pos = x->pos, step = x->step, len = x->len << 16; int gl = x->gl, gr = x->gr;
		int i;
		for (i = 0; i < NS && pos < len; i++, pos += step) { int s = d[pos >> 16]; mixed[i] += (s * gl) >> 8; mixed[NS + i] += (s * gr) >> 8; }
		x->pos = pos; if (pos >= len) { x->active = 0; }
	}
	if (mus.active && mus.d) {
		int vol = (mus.vol * music_master) / 127;                       /* 0..127 */
		int g = (vol * 256) / 127 * 3 / 4;
		for (int i = 0; i < NS; i++) {
			if (mus.pos >= mus.len) { if (mus.loop) mus.pos = 0; else { mus.active = 0; break; } }
			int s = (mus.d[mus.pos++] * g) >> 8; mixed[i] += s; mixed[NS + i] += s;
		}
		if (mus.fadems_left > 0) {                                      /* linear ramp per block */
			mus.vol += mus.fadestep; if (--mus.fadems_left == 0) { mus.vol = mus.fadeto; if (mus.fadeto == 0) mus.active = 0; }
			if (mus.vol < 0) mus.vol = 0; if (mus.vol > 127) mus.vol = 127;
		}
	}
	signed char *l = blk[fill][0], *r = blk[fill][1];
	for (int i = 0; i < NS; i++) { int a = mixed[i], b = mixed[NS + i]; l[i] = a > 127 ? 127 : a < -128 ? -128 : a; r[i] = b > 127 ? 127 : b < -128 ? -128 : b; }
	R16(AUD(0, 0)) = (unsigned)l >> 16; R16(AUD(0, 2)) = (unsigned)l & 0xffff;
	R16(AUD(1, 0)) = (unsigned)r >> 16; R16(AUD(1, 2)) = (unsigned)r & 0xffff;
	play = fill;
	R16(0x09C) = 0x0080;
}
extern void aud_isr_stub(void);
static struct __attribute__((packed)) { void *succ, *pred; unsigned char type, pri; char *name; void *data; void *code; } ais;
static void *old_vec;

void plat_audio_init(void)
{
	if (started) return; started = 1;
	for (int b = 0; b < 2; b++) for (int c = 0; c < 2; c++) blk[b][c] = AllocMem(NS, MEMF_CHIP | MEMF_CLEAR);
	for (int v = 0; v < NV; v++) vbuf[v] = AllocMem(VBUFSZ, MEMF_ANY);
	unsigned clock = plat_pal ? MIXRATE_PAL : 3579545u; unsigned per = 222; mixrate = clock / per;
	R16(0x096) = 0x0003; R16(0x09E) = 0x00FF; R16(0x09A) = 0x0080;
	ais.type = 2; ais.pri = 0; ais.name = "raptor.audio"; ais.code = (void *)aud_isr_stub;
	old_vec = SetIntVector(7, &ais);
	for (int c = 0; c < 2; c++) {
		R16(AUD(c, 0)) = (unsigned)blk[0][c] >> 16; R16(AUD(c, 2)) = (unsigned)blk[0][c] & 0xffff;
		R16(AUD(c, 4)) = NS / 2; R16(AUD(c, 6)) = per; R16(AUD(c, 8)) = 64;
	}
	play = 0;
	R16(0x09C) = 0x0080; R16(0x09A) = 0x8080;                  /* enable AUD0 interrupt */
	R16(0x096) = 0x8203;                                        /* DMAEN, AUD0, AUD1 */
}
void plat_audio_exit(void) { if (started) { R16(0x09A) = 0x0080; R16(0x096) = 0x0003; if (old_vec) SetIntVector(7, old_vec); started = 0; } }

/* ---- sound effects ---- */
static unsigned step_for(unsigned rate, int cents)
{
	static const unsigned f[13] = { 65536, 69433, 73562, 77936, 82570, 87480, 92682, 98193, 104032, 110218, 116772, 123715, 131072 };
	int semis = cents / 100, rem = cents % 100; if (rem < 0) { rem += 100; semis--; }
	int oct = 0; while (semis < 0) { semis += 12; oct--; } while (semis >= 12) { semis -= 12; oct++; }
	unsigned mul = f[semis] + (unsigned)(((unsigned long long)(f[semis + 1] - f[semis]) * rem) / 100);
	unsigned long long r = (unsigned long long)rate * mul;                        /* rate * 2^(cents/1200) in 16.16 */
	if (oct > 0) r <<= oct; else if (oct < 0) r >>= -oct;
	return (unsigned)((r / (unsigned long long)(mixrate ? mixrate : 15977)));    /* 16.16 source samples per output sample */
}
int SFX_PlayPatch(void *vdata, int pitch, int sep, int vol, int unk1, int priority)
{
	unsigned char *d = vdata;
	plat_audio_init();
	if (!d || (d[1] << 8 | d[0]) != 3) return -1;
	unsigned rate = (d[3] << 8) | d[2];
	unsigned len = (d[7] << 24) | (d[6] << 16) | (d[5] << 8) | d[4];
	if (len <= 48 || rate < 1000) return -1;
	len -= 32; if (len > VBUFSZ) len = VBUFSZ;
	int c = -1; unsigned long long old = ~0ull;
	for (int v = 0; v < NV; v++) if (!vox[v].active) { c = v; break; }
	if (c < 0) { for (int v = 0; v < NV; v++) if (vox[v].end < old) { old = vox[v].end; c = v; } }
	Voice *x = &vox[c];
	x->active = 0;                                                   /* stop while replacing */
	const unsigned char *s = d + 24; signed char *b = vbuf[c];
	for (unsigned i = 0; i < len; i++) b[i] = (signed char)(s[i] ^ 0x80);
	if (vol < 0) vol = 0; if (vol > 127) vol = 127;
	int gl = (vol * (254 - sep) * 256) / (127 * 190), gr = (vol * sep * 256) / (127 * 190);
	if (gl > 320) gl = 320; if (gr > 320) gr = 320; if (gl < 0) gl = 0; if (gr < 0) gr = 0;
	x->d = b; x->len = len; x->pos = 0; x->step = step_for(rate, ((pitch - 128) * 2400) / 128); x->gl = gl; x->gr = gr;
	x->id = next_id; x->end = plat_vbl_count;
	isr_log("CUE sfx id=%d voice=%d rate=%u len=%u gl=%d gr=%d pitch=%d t=%u\n", next_id, c, rate, len, gl, gr, pitch, plat_vbl_count);
	next_id = (next_id + 1) & 0x7fff; if (!next_id) next_id = 1;
	x->active = 1;
	return x->id;
}
int SFX_Playing(int id) { for (int v = 0; v < NV; v++) if (vox[v].id == id && vox[v].active) return 1; return 0; }
void SFX_StopPatch(int id) { for (int v = 0; v < NV; v++) if (vox[v].id == id) vox[v].active = 0; }

/* ---- music ---- */
int MUS_RegisterSong(void *data)
{
	unsigned char *m = data; plat_audio_init(); load_index();
	unsigned len = (m[5] << 8 | m[4]) + (m[7] << 8 | m[6]);                /* score length + start offset = bytes in the MUS lump */
	unsigned crc = crc32(m, len);
	mus.name[0] = 0;
	for (int i = 0; i < nidx; i++) if (idx[i].crc == crc) { int k = 0; for (; idx[i].name[k]; k++) mus.name[k] = idx[i].name[k]; mus.name[k] = 0; }
	isr_log("CUE mus register len=%u crc=%x name=%s t=%u\n", len, crc, mus.name, plat_vbl_count);
	return ++mus.id;
}
static void load_song(void)
{
	if (mus.loaded || !mus.name[0]) return;
	char path[48]; sprintf(path, "SYS:MUSIC/%s.PCM", mus.name);
	int fd = open(path, 0); if (fd < 0) { isr_log("CUE mus file missing %s\n", path); return; }
	long n = filelength(fd); signed char *p = AllocMem(n + 16, MEMF_ANY);
	if (p) { long got = 0; while (got < n) { int r = read(fd, p + got, n - got > 32768 ? 32768 : n - got); if (r <= 0) break; got += r; }
		mus.d = p; mus.len = got; mus.loaded = (int)(n + 16); }
	close(fd);
}
static void unload_song(void)
{
	mus.active = 0; signed char *p = mus.d; int sz = mus.loaded; mus.d = 0; mus.loaded = 0; mus.len = 0;
	if (p && sz) FreeMem(p, sz);
}
int MUS_UnregisterSong(int h) { isr_log("CUE mus unregister %d\n", h); unload_song(); return 0; }
int MUS_PlaySong(int h, int v) { load_song(); mus.pos = 0; mus.vol = v > 127 ? 127 : v; mus.fadems_left = 0; mus.active = mus.d != 0; isr_log("CUE mus play %d vol=%d t=%u\n", h, v, plat_vbl_count); return 0; }
int MUS_ChainSong(int h, int n) { mus.loop = (n == h); isr_log("CUE mus chain %d->%d\n", h, n); return 0; }
int MUS_FadeInSong(int h, int ms)
{
	load_song(); mus.pos = 0; int blocks = (ms * mixrate / 1000) / NS; if (blocks < 1) blocks = 1;
	mus.vol = 0; mus.fadeto = 127; mus.fadems_left = blocks; mus.fadestep = (127 + blocks - 1) / blocks; mus.active = mus.d != 0;
	isr_log("CUE mus fadein %d %dms t=%u\n", h, ms, plat_vbl_count); return 0;
}
int MUS_FadeOutSong(int h, int ms)
{
	int blocks = (ms * (mixrate ? mixrate : 15977) / 1000) / NS; if (blocks < 1) blocks = 1;
	mus.fadeto = 0; mus.fadems_left = blocks; mus.fadestep = -((mus.vol + blocks - 1) / blocks); if (!mus.fadestep) mus.fadestep = -1;
	isr_log("CUE mus fadeout %d %dms\n", h, ms); return 0;
}
int MUS_StopSong(int h) { mus.active = 0; mus.fadems_left = 0; isr_log("CUE mus stop %d\n", h); return 0; }
int MUS_QrySongPlaying(int h) { return mus.active; }
int MUS_SetMasterVolume(int v) { music_master = v < 0 ? 0 : v > 127 ? 127 : v; isr_log("CUE mus volume %d\n", v); return 0; }
