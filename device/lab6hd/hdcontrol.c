/* hdcontrol.c - k2023202316_hdcontrol */

#include <xinu.h>

devcall k2023202316_hdcontrol(struct dentry *devptr, int32 func, int32 arg1,
                              int32 arg2) {
  (void)devptr;
  (void)func;
  (void)arg1;
  (void)arg2;
  return OK;
}
