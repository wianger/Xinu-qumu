/* Lab5.h - experiment 5 keyboard and VGA console support */

#ifndef LAB5_H
#define LAB5_H

#define K2023202316_VGA_BASE 0x000b8000
#define K2023202316_VGA_COLS 80
#define K2023202316_VGA_ROWS 25
#define K2023202316_VGA_SIZE                                                    \
  (K2023202316_VGA_COLS * K2023202316_VGA_ROWS)
#define K2023202316_VGA_ATTR 0x07
#define K2023202316_VGA_RED_ATTR 0x0c
#define K2023202316_KBD_IBUFLEN 256
#define K2023202316_TAB_WIDTH 8

struct k2023202316_kbdvga_cblk {
  char *ihead;
  char *itail;
  char ibuff[K2023202316_KBD_IBUFLEN];
  uint8 iwid[K2023202316_KBD_IBUFLEN];
  uint8 irow[K2023202316_KBD_IBUFLEN];
  uint8 icol[K2023202316_KBD_IBUFLEN];
  sid32 isem;
  int32 icursor;
  int32 row;
  int32 col;
  uint8 attr;
  uint8 ansi_state;
  uint8 ansi_len;
  char ansi_buf[16];
  uint32 shift;
  bool8 initialized;
  bool8 sem_ready;
};

extern struct k2023202316_kbdvga_cblk k2023202316_kbdvga;

extern devcall k2023202316_kbdvgainit(struct dentry *);
extern devcall k2023202316_kbdvgaread(struct dentry *, void *, uint32);
extern devcall k2023202316_kbdvgawrite(struct dentry *, void *, uint32);
extern devcall k2023202316_kbdgetc(struct dentry *);
extern devcall k2023202316_vgaputc(struct dentry *, char);
extern devcall k2023202316_kbdvgactl(struct dentry *, int32, int32, int32);
extern syscall k2023202316_vga_polled_putc(byte);
extern syscall k2023202316_kbd_polled_getc(void);
extern void k2023202316_kbdhandler(void);
extern void k2023202316_vga_clear(void);
extern interrupt k2023202316_kbddisp(void);

#endif
