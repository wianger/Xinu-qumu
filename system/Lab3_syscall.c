/* Lab3_syscall.c - experiment 3/4 syscall dispatch */

#include <xinu.h>

/*------------------------------------------------------------------------
 * k2023202316_syscall_dispatch - dispatch int 0x80 requests
 *------------------------------------------------------------------------
 */
int32 k2023202316_syscall_dispatch(struct k2023202316_trapframe *tf) {
  uint32 sysno;
  uint32 a1;
  uint32 a2;
  uint32 a3;
  uint32 a4;
  uint32 a5;
  uint32 kargv[K2023202316_MAX_UARGS + 1];
  uint32 nargs;
  uint32 i;

  sysno = tf->eax;
  a1 = tf->ebx;
  a2 = tf->ecx;
  a3 = tf->edx;
  a4 = tf->esi;
  a5 = tf->edi;

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
    if (a5 == 0 ||
        k2023202316_copy_from_user(currpid, kargv, a5, sizeof(kargv)) ==
            SYSERR) {
      return SYSERR;
    }
    nargs = kargv[0];
    switch (nargs) {
    case 0:
      return k2023202316_create_user_proc((void *)a1, a2, (pri16)a3,
                                          (char *)a4, 0);
    case 1:
      return k2023202316_create_user_proc((void *)a1, a2, (pri16)a3,
                                          (char *)a4, 1, kargv[1]);
    case 2:
      return k2023202316_create_user_proc((void *)a1, a2, (pri16)a3,
                                          (char *)a4, 2, kargv[1], kargv[2]);
    case 3:
      return k2023202316_create_user_proc((void *)a1, a2, (pri16)a3,
                                          (char *)a4, 3, kargv[1], kargv[2],
                                          kargv[3]);
    case 4:
      return k2023202316_create_user_proc((void *)a1, a2, (pri16)a3,
                                          (char *)a4, 4, kargv[1], kargv[2],
                                          kargv[3], kargv[4]);
    case 5:
      return k2023202316_create_user_proc((void *)a1, a2, (pri16)a3,
                                          (char *)a4, 5, kargv[1], kargv[2],
                                          kargv[3], kargv[4], kargv[5]);
    default:
      return SYSERR;
    }

  case K2023202316_SYS_RESUME:
    return resume((pid32)a1);

  case K2023202316_SYS_RECEIVE:
    return receive();

  case K2023202316_SYS_FORK:
    return k2023202316_fork_from_trapframe(tf);

  case K2023202316_SYS_EXEC:
    if (a5 == 0 ||
        k2023202316_copy_from_user(currpid, kargv, a5, sizeof(kargv)) ==
            SYSERR) {
      return SYSERR;
    }
    nargs = kargv[0];
    if (nargs > K2023202316_MAX_UARGS) {
      return SYSERR;
    }
    for (i = 0; i < nargs; i++) {
      kargv[i] = kargv[i + 1];
    }
    return k2023202316_exec_from_trapframe(tf, (void *)a1, (pri16)a2,
                                           (char *)a3, nargs, kargv);

  case K2023202316_SYS_UMALLOC:
    return (int32)k2023202316_user_malloc(a1);

  case K2023202316_SYS_UFREE:
    return k2023202316_user_free((void *)a1);

  case K2023202316_SYS_GETPNAME:
    return k2023202316_getpname((pid32)a1, (char *)a2, a3);

  case K2023202316_SYS_WRITEFILE: // Lab6 2023202316
    return k2023202316_user_write_file((char *)a1, (char *)a2, a3);

  case K2023202316_SYS_EXECFILE: // Lab6 2023202316
    return k2023202316_execfile_from_user(tf, (char *)a1, a2, (char **)a3);

  default:
    return SYSERR;
  }
}
