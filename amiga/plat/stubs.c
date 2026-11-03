/* Sound hardware is not implemented yet: all card detection fails, music/sfx calls do nothing. */
#define STUB(n) int n() { return 0; }
int DMX_Init() { return 1; }
STUB(DMX_DeInit) STUB(AL_Detect) STUB(AL_SetCard) STUB(GF1_Detect) STUB(GF1_SetMap) STUB(MPU_Detect) STUB(MPU_SetCard)
STUB(MUS_QrySongPlaying) 
STUB(MV_Detect) STUB(SB_Detect) STUB(SB_SetCard)
STUB(TS_Dispatch) STUB(TS_ScheduleTask) STUB(TS_Shutdown) STUB(TS_Terminate) STUB(WAV_PlayMode)

extern void isr_log(const char *f, ...);
extern volatile unsigned plat_vbl_count;
static int mus_cur;
int MUS_RegisterSong(void *d) { isr_log("CUE mus register data=%x t=%u\n", (unsigned)d, plat_vbl_count); return ++mus_cur; }
int MUS_UnregisterSong(int h) { isr_log("CUE mus unregister %d\n", h); return 0; }
int MUS_PlaySong(int h, int v) { isr_log("CUE mus play %d vol=%d t=%u\n", h, v, plat_vbl_count); return 0; }
int MUS_ChainSong(int h, int n) { isr_log("CUE mus chain %d->%d\n", h, n); return 0; }
int MUS_FadeInSong(int h, int ms) { isr_log("CUE mus fadein %d %dms t=%u\n", h, ms, plat_vbl_count); return 0; }
int MUS_FadeOutSong(int h, int ms) { isr_log("CUE mus fadeout %d %dms\n", h, ms); return 0; }
int MUS_StopSong(int h) { isr_log("CUE mus stop %d\n", h); return 0; }
int MUS_SetMasterVolume(int v) { isr_log("CUE mus volume %d\n", v); return 0; }
