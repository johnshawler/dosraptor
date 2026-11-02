#ifndef PLAT_H
#define PLAT_H
extern volatile unsigned plat_vbl_count;
extern unsigned char *plat_vram;           /* 320x200 chunky "VGA memory" */
void plat_video_open(void);
void plat_video_close(void);
void plat_set_color(int idx, int r6, int g6, int b6);
void plat_present(void);
void plat_mouse_set(int x, int y);
void plat_mouse_event(int buttons, int x, int y);
void plat_poll_input(void);
void plat_vbl_hw(void);
void plat_tsm_init(void);
void plat_tsm_exit(void);
extern void KBD_ReadScan(int key);
unsigned plat_joy(void);                    /* bit0 right? see video.c */
#endif
