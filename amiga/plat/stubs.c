/* Sound hardware is not implemented yet: all card detection fails, music/sfx calls do nothing. */
#define STUB(n) int n() { return 0; }
STUB(DMX_DeInit) STUB(AL_Detect) STUB(AL_SetCard) STUB(DMX_Init) STUB(GF1_Detect) STUB(GF1_SetMap) STUB(MPU_Detect) STUB(MPU_SetCard)
STUB(MUS_ChainSong) STUB(MUS_FadeInSong) STUB(MUS_FadeOutSong) STUB(MUS_PlaySong) STUB(MUS_QrySongPlaying) STUB(MUS_RegisterSong)
STUB(MUS_SetMasterVolume) STUB(MUS_StopSong) STUB(MUS_UnregisterSong) STUB(MV_Detect) STUB(SB_Detect) STUB(SB_SetCard)
STUB(SFX_PlayPatch) STUB(SFX_Playing) STUB(SFX_StopPatch) STUB(TS_Dispatch) STUB(TS_ScheduleTask) STUB(TS_Shutdown) STUB(TS_Terminate) STUB(WAV_PlayMode)
