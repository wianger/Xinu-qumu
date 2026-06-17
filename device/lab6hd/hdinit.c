/* hdinit.c - k2023202316_hdinit */

#include <xinu.h>

struct k2023202316_hd_cblk k2023202316_hd;

/*------------------------------------------------------------------------
 * k2023202316_hdinit - initialize the primary-channel slave ATA disk
 *------------------------------------------------------------------------
 */
devcall k2023202316_hdinit(struct dentry *devptr) {
  k2023202316_hd.lock = semcreate(1);
  k2023202316_hd.done = semcreate(0);
  k2023202316_hd.present = TRUE;
  k2023202316_hd.waiting = FALSE;
  k2023202316_hd.error = FALSE;
  k2023202316_hd.status = 0;
  set_evec(devptr->dvirq, (uint32)devptr->dvintr);
  return OK;
}
