/* hdwrite.c - k2023202316_hdwrite */

#include <xinu.h>

extern status k2023202316_hd_xfer(struct dentry *, void *, uint32, bool8);

/*------------------------------------------------------------------------
 * k2023202316_hdwrite - write one 512-byte sector to the disk.
 *
 * The generic write() count argument is used as the sector LBA, matching
 * device/ram's block-number convention.  buff must point to 512 bytes.
 *------------------------------------------------------------------------
 */
devcall k2023202316_hdwrite(struct dentry *devptr, void *buff, uint32 blk) {
  return k2023202316_hd_xfer(devptr, buff, blk, TRUE);
}
