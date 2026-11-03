/* Sound hardware is not implemented yet: all card detection fails, music/sfx calls do nothing. */
#define STUB(n) int n() { return 0; }
int DMX_Init() { return 1; }
STUB(DMX_DeInit) STUB(AL_Detect) STUB(AL_SetCard) STUB(GF1_Detect) STUB(GF1_SetMap) STUB(MPU_Detect) STUB(MPU_SetCard)

STUB(MV_Detect) STUB(SB_Detect) STUB(SB_SetCard)
STUB(TS_Dispatch) STUB(TS_ScheduleTask) STUB(TS_Shutdown) STUB(TS_Terminate) STUB(WAV_PlayMode)
