/* xsh_lab3.c - xsh_lab3 */

#include <xinu.h>
#include <stdio.h>
#include <string.h>

#define K2023202316_NAME "wangyihang"
#define K2023202316_ID "2023202316"

local process u2023202316_uptest(int32, int32);
local status u2023202316_run_uptest(char *, int32, int32);

/*------------------------------------------------------------------------
 * xsh_lab3 - run experiment 3 user mode process tests
 *------------------------------------------------------------------------
 */
shellcmd xsh_lab3(int nargs, char *args[]) {
  pid32 pid;
  status result;

  (void)nargs;

  if (nargs == 2 && strncmp(args[1], "--help", 7) == 0) {
    if (u2023202316_getcpl() == 3) {
      u2023202316_printf("Use:\n");
      u2023202316_printf("\t%s\n", args[0]);
      u2023202316_printf("Description:\n");
      u2023202316_printf("\tRun experiment 3 user mode process tests\n");
      u2023202316_printf("Options:\n");
      u2023202316_printf("\t--help\tdisplay this help and exit\n");
    } else {
      printf("Use:\n");
      printf("\t%s\n", args[0]);
      printf("Description:\n");
      printf("\tRun experiment 3 user mode process tests\n");
      printf("Options:\n");
      printf("\t--help\tdisplay this help and exit\n");
    }
    return SHELL_OK;
  }

  pid = u2023202316_getpid();
  u2023202316_printf("xsh_lab3: pid=%d name=%s cpl=%d\n", pid,
                     proctab[pid].prname, u2023202316_getcpl());

  result = OK;
  if (u2023202316_run_uptest("uptest-1", 2023, 16) == SYSERR) {
    result = SYSERR;
  }
  if (u2023202316_run_uptest("uptest-2", 100, 200) == SYSERR) {
    result = SYSERR;
  }
  if (u2023202316_run_uptest("uptest-3", 7, 99) == SYSERR) {
    result = SYSERR;
  }

  u2023202316_printf("lab3 done: %s %s\n", K2023202316_NAME, K2023202316_ID);
  return (result == OK) ? SHELL_OK : SHELL_ERROR;
}

local status u2023202316_run_uptest(char *name, int32 a, int32 b) {
  pid32 pid;
  umsg32 msg;

  pid = u2023202316_create_user_proc((void *)u2023202316_uptest,
                                     K2023202316_USER_STK, INITPRIO, name, 2,
                                     a, b);
  if (pid == SYSERR) {
    u2023202316_printf("create %s failed\n", name);
    return SYSERR;
  }

  u2023202316_printf("created user proc pid=%d name=%s\n", pid,
                     proctab[pid].prname);
  if (u2023202316_resume(pid) == SYSERR) {
    u2023202316_printf("resume %s failed\n", name);
    return SYSERR;
  }

  do {
    msg = u2023202316_receive();
  } while (msg != (umsg32)pid);

  return OK;
}

local process u2023202316_uptest(int32 a, int32 b) {
  pid32 pid;

  pid = u2023202316_getpid();
  u2023202316_printf("proc=%d name=%s: a=%d\n", pid, proctab[pid].prname, a);
  u2023202316_sleepms(10);
  u2023202316_printf("proc=%d name=%s: b=%d\n", pid, proctab[pid].prname, b);
  return OK;
}
