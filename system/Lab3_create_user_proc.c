/* Lab3_create_user_proc.c - create a user mode process */

#include <xinu.h>
#include <stdarg.h>

local pid32 k2023202316_newpid(void);
local void k2023202316_set_name(struct procent *, char *);
local char *k2023202316_build_kernel_stack(char *, void *, char *);

/*------------------------------------------------------------------------
 * k2023202316_create_user_proc - create a suspended user mode process
 *------------------------------------------------------------------------
 */
pid32 k2023202316_create_user_proc(void *funcaddr, uint32 ssize,
                                   pri16 priority, char *name, uint32 nargs,
                                   ...) {
  intmask mask;
  pid32 pid;
  struct procent *prptr;
  uint32 i;
  va_list ap;
  uint32 args[K2023202316_MAX_UARGS];
  char *kstkbase;
  uint32 uesp;

  (void)ssize;
  (void)priority;

  /*Lab4 2023202316: Begin*/
  if ((uint32)funcaddr < (uint32)&k2023202316_usertext ||
      (uint32)funcaddr >= (uint32)&k2023202316_eusertext ||
      (nargs > K2023202316_MAX_UARGS)) {
    return SYSERR;
  }
  /*Lab4 2023202316: End*/

  for (i = 0; i < K2023202316_MAX_UARGS; i++) {
    args[i] = 0;
  }

  va_start(ap, nargs);
  for (i = 0; i < nargs; i++) {
    args[i] = va_arg(ap, uint32);
  }
  va_end(ap);

  mask = disable();
  pid = k2023202316_newpid();
  if (pid == SYSERR) {
    restore(mask);
    return SYSERR;
  }

  kstkbase = getstk(K2023202316_KERNEL_STK);
  if (kstkbase == (char *)SYSERR) {
    restore(mask);
    return SYSERR;
  }

  prcount++;
  prptr = &proctab[pid];
  prptr->prstate = PR_SUSP;
  prptr->prprio = INITPRIO;
  prptr->prstkbase = kstkbase;
  prptr->prstklen = K2023202316_KERNEL_STK;
  prptr->prsem = -1;
  prptr->prparent = (pid32)getpid();
  prptr->prhasmsg = FALSE;
  prptr->prmsg = 0;
  prptr->pr2023202316_isuser = TRUE;
  k2023202316_init_proc_vm_fields(prptr); // Lab4 2023202316
  k2023202316_set_name(prptr, name);

  if (k2023202316_create_addrspace(pid) == 0 ||
      k2023202316_map_user_stack(pid, K2023202316_USER_STK) == SYSERR) {
    k2023202316_free_user_space(pid); // Lab4 2023202316
    freestk(kstkbase, K2023202316_KERNEL_STK);
    prptr->prstate = PR_FREE;
    prcount--;
    restore(mask);
    return SYSERR;
  }
  uesp = k2023202316_build_user_stack(pid, (void *)u2023202316_exit, nargs,
                                      args);
  if (uesp == 0) {
    k2023202316_free_user_space(pid);
    freestk(kstkbase, K2023202316_KERNEL_STK);
    prptr->prstate = PR_FREE;
    prcount--;
    restore(mask);
    return SYSERR;
  }
  prptr->prstkptr =
      k2023202316_build_kernel_stack(kstkbase, funcaddr, (char *)uesp);

  for (i = 0; i < NDESC; i++) {
    prptr->prdesc[i] = -1;
  }
  if (!isbadpid(currpid)) {
    for (i = 0; i < NDESC; i++) {
      prptr->prdesc[i] = proctab[currpid].prdesc[i];
    }
  } else {
    prptr->prdesc[0] = CONSOLE;
    prptr->prdesc[1] = CONSOLE;
    prptr->prdesc[2] = CONSOLE;
  }

  restore(mask);
  return pid;
}

local char *k2023202316_build_kernel_stack(char *stkbase, void *funcaddr,
                                           char *uesp) {
  uint32 savsp;
  uint32 *pushsp;
  uint32 *saddr;

  saddr = (uint32 *)stkbase;
  *saddr = STACKMAGIC;
  savsp = (uint32)saddr;

  *--saddr = (uint32)uesp;
  *--saddr = (uint32)funcaddr;
  *--saddr = (uint32)INITRET;
  *--saddr = (uint32)k2023202316_iret_to_user;
  *--saddr = savsp;
  savsp = (uint32)saddr;
  *--saddr = 0x00000200;
  *--saddr = 0;
  *--saddr = 0;
  *--saddr = 0;
  *--saddr = 0;
  *--saddr = 0;
  pushsp = saddr;
  *--saddr = savsp;
  *--saddr = 0;
  *--saddr = 0;
  *pushsp = (uint32)saddr;

  return (char *)saddr;
}

local void k2023202316_set_name(struct procent *prptr, char *name) {
  int32 i;

  if (name == NULL) {
    name = "userproc";
  }
  prptr->prname[PNMLEN - 1] = NULLCH;
  for (i = 0; i < PNMLEN - 1 && name[i] != NULLCH; i++) {
    prptr->prname[i] = name[i];
  }
  if (i < PNMLEN) {
    prptr->prname[i] = NULLCH;
  }
}

local pid32 k2023202316_newpid(void) {
  uint32 i;
  static pid32 nextpid = 1;

  for (i = 0; i < NPROC; i++) {
    nextpid %= NPROC;
    if (proctab[nextpid].prstate == PR_FREE) {
      return nextpid++;
    }
    nextpid++;
  }
  return (pid32)SYSERR;
}
