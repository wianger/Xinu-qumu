/* write.c - write */

#include <xinu.h>

/*------------------------------------------------------------------------
 *  write  -  Write one or more bytes to a device
 *------------------------------------------------------------------------
 */
syscall write(did32 descrp, /* Descriptor for device	*/
              char *buffer, /* Address of buffer		*/
              uint32 count  /* Length of buffer		*/
) {
  intmask mask;          /* Saved interrupt mask		*/
  struct dentry *devptr; /* Entry in device switch table	*/
  int32 retval;          /* Value to return to caller	*/

  mask = disable();
  if (isbaddev(descrp)) {
    restore(mask);
    return SYSERR;
  }
  devptr = (struct dentry *)&devtab[descrp];
  /*Lab6 2023202316: Begin*/
#ifdef HD0
  if (descrp == HD0) {
    restore(mask);
    return (*devptr->dvwrite)(devptr, buffer, count);
  }
#endif
  /*Lab6 2023202316: End*/
  retval = (*devptr->dvwrite)(devptr, buffer, count);
  restore(mask);
  return retval;
}
