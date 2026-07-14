/* hdread.c - k2023202316_hdread */

#include <xinu.h>

#define K2023202316_ATA_DATA 0x1F0
#define K2023202316_ATA_ERROR 0x1F1
#define K2023202316_ATA_SECCNT 0x1F2
#define K2023202316_ATA_LBA0 0x1F3
#define K2023202316_ATA_LBA1 0x1F4
#define K2023202316_ATA_LBA2 0x1F5
#define K2023202316_ATA_DRVHD 0x1F6
#define K2023202316_ATA_STATUS 0x1F7
#define K2023202316_ATA_CMD 0x1F7

#define K2023202316_ATA_SR_ERR 0x01
#define K2023202316_ATA_SR_DRQ 0x08
#define K2023202316_ATA_SR_DF 0x20
#define K2023202316_ATA_SR_RDY 0x40
#define K2023202316_ATA_SR_BSY 0x80

#define K2023202316_ATA_CMD_READ 0x20

local status k2023202316_hd_wait_ready(void);
local status k2023202316_hd_wait_irq(void);
extern status k2023202316_hd_xfer(struct dentry *, void *, uint32, bool8);

/*------------------------------------------------------------------------
 * k2023202316_hdread - read one 512-byte sector from the disk.
 *
 * The generic read() count argument is used as the sector LBA, matching
 * device/ram's block-number convention.  buff must point to 512 bytes.
 *------------------------------------------------------------------------
 */
devcall k2023202316_hdread(struct dentry *devptr, void *buff, uint32 blk) {
  return k2023202316_hd_xfer(devptr, buff, blk, FALSE);
}

/*------------------------------------------------------------------------
 * k2023202316_hd_xfer - PIO sector transfer completed by IRQ14
 *------------------------------------------------------------------------
 */
status k2023202316_hd_xfer(struct dentry *devptr, void *buff, uint32 blk,
                           bool8 writeop) {
  intmask mask;
  uint16 *data;
  uint32 i;
  uint8 status;

  if (buff == NULL || !k2023202316_hd.present) {
    return SYSERR;
  }

  wait(k2023202316_hd.lock);
  semreset(k2023202316_hd.done, 0);
  mask = disable();
  if (k2023202316_hd_wait_ready() == SYSERR) {
    restore(mask);
    signal(k2023202316_hd.lock);
    return SYSERR;
  }

  k2023202316_hd.waiting = TRUE;
  k2023202316_hd.error = FALSE;
  outb(K2023202316_ATA_DRVHD, 0xE0 | 0x10 | ((blk >> 24) & 0x0F));
  outb(K2023202316_ATA_SECCNT, 1);
  outb(K2023202316_ATA_LBA0, blk & 0xFF);
  outb(K2023202316_ATA_LBA1, (blk >> 8) & 0xFF);
  outb(K2023202316_ATA_LBA2, (blk >> 16) & 0xFF);

  if (writeop) {
    outb(K2023202316_ATA_CMD, 0x30);
    if (k2023202316_hd_wait_ready() == SYSERR) {
      k2023202316_hd.waiting = FALSE;
      restore(mask);
      signal(k2023202316_hd.lock);
      return SYSERR;
    }
    status = inb(K2023202316_ATA_STATUS);
    if (!(status & K2023202316_ATA_SR_DRQ)) {
      k2023202316_hd.waiting = FALSE;
      restore(mask);
      signal(k2023202316_hd.lock);
      return SYSERR;
    }
    data = (uint16 *)buff;
    for (i = 0; i < K2023202316_HD_SECSIZE / sizeof(uint16); i++) {
      outw(K2023202316_ATA_DATA, data[i]);
    }
    restore(mask);
    if (k2023202316_hd_wait_irq() == SYSERR) {
      signal(k2023202316_hd.lock);
      return SYSERR;
    }
  } else {
    outb(K2023202316_ATA_CMD, K2023202316_ATA_CMD_READ);
    restore(mask);
    if (k2023202316_hd_wait_irq() == SYSERR) {
      signal(k2023202316_hd.lock);
      return SYSERR;
    }
    mask = disable();
    status = inb(K2023202316_ATA_STATUS);
    if (status & (K2023202316_ATA_SR_ERR | K2023202316_ATA_SR_DF)) {
      k2023202316_hd.error = TRUE;
    } else if (status & K2023202316_ATA_SR_DRQ) {
      data = (uint16 *)buff;
      for (i = 0; i < K2023202316_HD_SECSIZE / sizeof(uint16); i++) {
        data[i] = inw(K2023202316_ATA_DATA);
      }
    } else {
      k2023202316_hd.error = TRUE;
    }
    restore(mask);
  }

  signal(k2023202316_hd.lock);
  return k2023202316_hd.error ? SYSERR : OK;
}

local status k2023202316_hd_wait_ready(void) {
  uint32 limit;
  uint8 status;

  for (limit = 0; limit < K2023202316_HD_READY_TIMEOUT; limit++) {
    status = inb(K2023202316_ATA_STATUS);
    if (!(status & K2023202316_ATA_SR_BSY) &&
        (status & K2023202316_ATA_SR_RDY)) {
      return OK;
    }
  }
  return SYSERR;
}

local status k2023202316_hd_wait_irq(void) {
  uint32 elapsed;
  intmask mask;

  for (elapsed = 0; elapsed < K2023202316_HD_IRQ_TIMEOUT_MS; elapsed++) {
    if (!k2023202316_hd.waiting) {
      wait(k2023202316_hd.done);
      return k2023202316_hd.error ? SYSERR : OK;
    }
    sleepms(1);
  }

  mask = disable();
  k2023202316_hd.waiting = FALSE;
  k2023202316_hd.error = TRUE;
  semreset(k2023202316_hd.done, 0);
  restore(mask);
  return SYSERR;
}
