/* hdhandler.c - k2023202316_hdhandler */

#include <xinu.h>

#define K2023202316_ATA_STATUS 0x1F7
#define K2023202316_ATA_SR_ERR 0x01
#define K2023202316_ATA_SR_DF 0x20

/*------------------------------------------------------------------------
 * k2023202316_hdhandler - IRQ14 completion handler for IDE requests
 *------------------------------------------------------------------------
 */
void k2023202316_hdhandler(void) {
  k2023202316_hd.status = inb(K2023202316_ATA_STATUS);
  if (k2023202316_hd.status &
      (K2023202316_ATA_SR_ERR | K2023202316_ATA_SR_DF)) {
    k2023202316_hd.error = TRUE;
  }
  if (k2023202316_hd.waiting) {
    k2023202316_hd.waiting = FALSE;
    signal(k2023202316_hd.done);
  }
}
