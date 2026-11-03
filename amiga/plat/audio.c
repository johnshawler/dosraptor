/* Sound effects on Paula: DMX digital samples (type 3: 8-bit unsigned PCM) on the four hardware channels.
   Channels 0 and 3 are the left, 1 and 2 the right output.  The OS audio.device is not in use by this program, so the
   registers are driven directly; each sample is copied to chip RAM and played once (the repeat pointer is switched to silence). */
#include "os.h"
#include "plat.h"
#define R16(o) (*(volatile unsigned short *)(0xDFF000 + (o)))
#define AUD(c, r) (0x0A0 + (c) * 0x10 + (r))
#define BUFSZ 40000
extern volatile unsigned plat_vbl_count;
void isr_log(const char *f, ...);
extern int plat_vbl_hz;
extern int plat_pal;
void *memcpy(void *, const void *, unsigned);

static unsigned char *buf[4], *silence;
static struct { unsigned end; int id; } ch[4];
static int next_id = 1, ready;

static void init_audio(void)
{
	if (ready) return; ready = 1;
	for (int i = 0; i < 4; i++) buf[i] = AllocMem(BUFSZ, MEMF_CHIP | MEMF_CLEAR);
	silence = AllocMem(8, MEMF_CHIP | MEMF_CLEAR);
	R16(0x09E) = 0x00FF;                  /* ADKCON: no modulation */
	R16(0x096) = 0x000F;                  /* audio DMA off */
}
static unsigned period_for(unsigned rate, int cents)
{
	/* period = clock / (rate * 2^(cents/1200)) */
	unsigned clock = plat_pal ? 3546895u : 3579545u;
	static const unsigned f[12] = { 65536, 69433, 73562, 77936, 82570, 87480, 92682, 98193, 104032, 110218, 116772, 123715 };   /* 2^(n/12) in 16.16 */
	int semis = cents / 100; int rem = cents % 100; if (rem < 0) { rem += 100; semis--; }
	int oct = 0; while (semis < 0) { semis += 12; oct--; } while (semis >= 12) { semis -= 12; oct++; }
	unsigned long long m = f[semis];
	unsigned mul = (unsigned)m + (((f[(semis + 1) % 12] * (semis == 11 ? 2 : 1)) - (unsigned)m) * rem) / 100;
	unsigned long long r = ((unsigned long long)rate * mul) >> 16;
	if (oct > 0) r <<= oct; else if (oct < 0) r >>= -oct;
	if (r < 1) r = 1;
	unsigned p = clock / (unsigned)r;
	return p < 124 ? 124 : p > 65535 ? 65535 : p;
}
int SFX_PlayPatch(void *vdata, int pitch, int sep, int vol, int unk1, int priority)
{
	unsigned char *d = vdata;
	init_audio();
	if (!d || (d[1] << 8 | d[0]) != 3) return -1;
	unsigned rate = (d[3] << 8) | d[2];
	unsigned len = (d[7] << 24) | (d[6] << 16) | (d[5] << 8) | d[4];
	if (len <= 48 || rate < 1000) return -1;
	len -= 32; if (len > BUFSZ - 4) len = BUFSZ - 4; len &= ~1u;
	unsigned now = plat_vbl_count;
	int left = sep < 127, c = -1, best = -1; unsigned oldest = 0xffffffff;
	int order[4] = { left ? 0 : 1, left ? 3 : 2, left ? 1 : 0, left ? 2 : 3 };
	for (int k = 0; k < 4 && c < 0; k++) if (ch[order[k]].end <= now) c = order[k];
	if (c < 0) { for (int k = 0; k < 4; k++) if (ch[k].end < oldest) { oldest = ch[k].end; best = k; } c = best; }
	R16(0x096) = 1 << c;                                   /* stop the channel */
	const unsigned char *s = d + 24; unsigned char *b = buf[c];
	for (unsigned i = 0; i < len; i++) b[i] = s[i] ^ 0x80;
	b[len] = b[len + 1] = 0;
	int v = (vol * 64) / 127; if (v > 64) v = 64; if (v < 0) v = 0;
	R16(AUD(c, 0)) = (unsigned)b >> 16; R16(AUD(c, 2)) = (unsigned)b & 0xffff;
	R16(AUD(c, 4)) = len / 2; R16(AUD(c, 6)) = period_for(rate, ((pitch - 128) * 2400) / 128); R16(AUD(c, 8)) = v;
	R16(0x09C) = 0x0080 << c;                              /* clear the audio interrupt flag */
	R16(0x096) = 0x8000 | 0x0200 | (1 << c);               /* DMAEN + channel on */
	for (int n = 0; n < 4000 && !(R16(0x01E) & (0x0080 << c)); n++) ;
	R16(AUD(c, 0)) = (unsigned)silence >> 16; R16(AUD(c, 2)) = (unsigned)silence & 0xffff; R16(AUD(c, 4)) = 2;   /* after one pass: silence */
	isr_log("CUE sfx id=%d ch=%d rate=%u len=%u vol=%d sep=%d pitch=%d t=%u\n", next_id, c, rate, len, v, sep, pitch, now);
	ch[c].id = next_id; ch[c].end = now + (unsigned)((unsigned long long)len * plat_vbl_hz / rate) + 2;
	next_id = (next_id + 1) & 0x7fff; if (!next_id) next_id = 1;
	return ch[c].id;
}
int SFX_Playing(int id) { for (int c = 0; c < 4; c++) if (ch[c].id == id && ch[c].end > plat_vbl_count) return 1; return 0; }
void SFX_StopPatch(int id) { for (int c = 0; c < 4; c++) if (ch[c].id == id) { R16(0x096) = 1 << c; ch[c].end = 0; } }
void SFX_StopAll(void) { R16(0x096) = 0x000F; for (int c = 0; c < 4; c++) ch[c].end = 0; }
