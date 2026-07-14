/* Lab6.h - experiment 6 disk, simple FS, and external ELF support */

#ifndef LAB6_H
#define LAB6_H

#include <elf.h>

struct k2023202316_trapframe;

#define K2023202316_ID "2023202316"
#define K2023202316_NAME "wangyihang"

#define K2023202316_HD_SECSIZE 512
#define K2023202316_HD_READY_TIMEOUT 10000000
#define K2023202316_HD_IRQ_TIMEOUT_MS 5000

#define K2023202316_FS_NAME_LEN 128
#define K2023202316_FS_IMAGE_SIZE (500 * 1024)
#define K2023202316_FS_MAX_FILES (K2023202316_FS_IMAGE_SIZE / 144 - 1)

#define K2023202316_ELF_BASE 0x50000000
#define K2023202316_ELF_LIMIT 0x70000000
#define K2023202316_EXT_MAX_ARGS 16
#define K2023202316_EXT_ARG_BYTES 512

#define K2023202316_SYS_WRITEFILE 13

struct k2023202316_hd_cblk {
  sid32 lock;
  sid32 done;
  bool8 present;
  bool8 waiting;
  bool8 error;
  uint8 status;
};

struct k2023202316_fs_dirent {
  uint32 offset;
  uint32 size;
  char name[K2023202316_FS_NAME_LEN];
};

extern struct k2023202316_hd_cblk k2023202316_hd;

extern devcall k2023202316_hdinit(struct dentry *);
extern devcall k2023202316_hdread(struct dentry *, void *, uint32);
extern devcall k2023202316_hdwrite(struct dentry *, void *, uint32);
extern devcall k2023202316_hdcontrol(struct dentry *, int32, int32, int32);
extern void k2023202316_hdhandler(void);
extern interrupt k2023202316_hddisp(void);

extern status k2023202316_fs_list(void);
extern status k2023202316_fs_find(char *, struct k2023202316_fs_dirent *);
extern int32 k2023202316_fs_read(char *, uint32, void *, uint32);
extern int32 k2023202316_fs_write(char *, uint32, void *, uint32);
extern syscall k2023202316_user_write_file(char *, char *, uint32);
extern syscall k2023202316_execfile_from_user(struct k2023202316_trapframe *,
                                              char *, uint32, char **);
extern syscall k2023202316_map_user_region(pid32, uint32, uint32, char *);

extern syscall u2023202316_writefile(char *, char *, uint32);
extern syscall u2023202316_execfile(char *, uint32, char **);

#endif
