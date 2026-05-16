/* Lab3_syscall.c - experiment 3 syscall dispatch */

#include <xinu.h>

/*------------------------------------------------------------------------
 * k2023202316_syscall_dispatch - dispatch int 0x80 requests
 *------------------------------------------------------------------------
 */
int32 k2023202316_syscall_dispatch(uint32 sysno, uint32 a1, uint32 a2,
                                   uint32 a3, uint32 a4, uint32 a5) {
  uint32 *argv;
  uint32 nargs;

  switch (sysno) {
  case K2023202316_SYS_GETPID:
    return getpid();

  case K2023202316_SYS_PUTC:
    return putc((did32)a1, (char)a2);

  case K2023202316_SYS_SLEEPMS:
    return sleepms((int32)a1);

  case K2023202316_SYS_EXIT:
    kill(getpid());
    return OK;

  case K2023202316_SYS_CREATE_USER_PROC:
    argv = (uint32 *)a5;
    if (argv == NULL) {
      return SYSERR;
    }
    nargs = argv[0];
    switch (nargs) {
    case 0:
      return k2023202316_create_user_proc((void *)a1, a2, (pri16)a3,
                                          (char *)a4, 0);
    case 1:
      return k2023202316_create_user_proc((void *)a1, a2, (pri16)a3,
                                          (char *)a4, 1, argv[1]);
    case 2:
      return k2023202316_create_user_proc((void *)a1, a2, (pri16)a3,
                                          (char *)a4, 2, argv[1], argv[2]);
    case 3:
      return k2023202316_create_user_proc((void *)a1, a2, (pri16)a3,
                                          (char *)a4, 3, argv[1], argv[2],
                                          argv[3]);
    case 4:
      return k2023202316_create_user_proc((void *)a1, a2, (pri16)a3,
                                          (char *)a4, 4, argv[1], argv[2],
                                          argv[3], argv[4]);
    case 5:
      return k2023202316_create_user_proc((void *)a1, a2, (pri16)a3,
                                          (char *)a4, 5, argv[1], argv[2],
                                          argv[3], argv[4], argv[5]);
    default:
      return SYSERR;
    }

  case K2023202316_SYS_RESUME:
    return resume((pid32)a1);

  case K2023202316_SYS_RECEIVE:
    return receive();

  default:
    return SYSERR;
  }
}
