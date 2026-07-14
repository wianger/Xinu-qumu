/* xsh_lab4.c - xsh_lab4 */

#include <xinu.h>
#include <stdio.h>
#include <string.h>

#define K2023202316_NAME "wangyihang"
#define K2023202316_ID "2023202316"

local status u2023202316_fork_test(void);
local status u2023202316_exec_test(void);
local process u2023202316_exec_target(int32, int32);
local status u2023202316_heap_test(void);
local status u2023202316_stack_test(void);
local void u2023202316_stack_touch(int32);
local void u2023202316_print_ctx(char *, int32 *);

shellcmd xsh_lab4(int nargs, char *args[]) {
  int32 x;
  status result;
  pid32 pid;
  volatile uint32 spin;
  char pname[PNMLEN];

  pid = u2023202316_getpid();
  u2023202316_getpname(pid, pname, sizeof(pname));
  x = 2023202316;
  u2023202316_printf("xsh_lab4: pid=%d name=%s cpl=%d &x=0x%08X\n", pid,
                     pname, u2023202316_getcpl(), &x);

  result = OK;
  if (nargs == 2 && strncmp(args[1], "map", 4) == 0) {
    u2023202316_printf("[lab4 map] inspect QEMU 'info mem' now\n");
    for (spin = 0; spin < 500000000; spin++) {
      asm volatile("" : : : "memory");
    }
  } else if (nargs == 2 && strncmp(args[1], "1", 2) == 0) {
    result = u2023202316_fork_test();
  } else if (nargs == 2 && strncmp(args[1], "2", 2) == 0) {
    result = u2023202316_exec_test();
  } else if (nargs == 2 && strncmp(args[1], "heap", 5) == 0) {
    result = u2023202316_heap_test();
  } else if (nargs == 2 && strncmp(args[1], "stack", 6) == 0) {
    result = u2023202316_stack_test();
  } else {
    if (u2023202316_fork_test() == SYSERR) {
      result = SYSERR;
    }
    if (u2023202316_exec_test() == SYSERR) {
      result = SYSERR;
    }
    if (u2023202316_heap_test() == SYSERR) {
      result = SYSERR;
    }
    if (u2023202316_stack_test() == SYSERR) {
      result = SYSERR;
    }
  }

  u2023202316_printf("%s %s\n", K2023202316_ID, K2023202316_NAME);
  return (result == OK) ? SHELL_OK : SHELL_ERROR;
}

local status u2023202316_fork_test(void) {
  pid32 pid;
  umsg32 msg;
  int32 value;

  value = 111;
  u2023202316_printf("[lab4 fork] before fork &value=0x%08X\n", &value);
  pid = u2023202316_fork();
  if (pid == SYSERR) {
    u2023202316_printf("[lab4 fork] fork failed\n");
    return SYSERR;
  }
  u2023202316_print_ctx(pid == 0 ? "fork-child" : "fork-parent", &value);
  if (pid == 0) {
    u2023202316_exit();
    return OK;
  }
  if (pid > 0) {
    do {
      msg = u2023202316_receive();
    } while (msg != (umsg32)pid);
  }
  return OK;
}

local status u2023202316_exec_test(void) {
  pid32 pid;
  umsg32 msg;
  int32 value;

  value = 222;
  pid = u2023202316_fork();
  if (pid == 0) {
    u2023202316_print_ctx("exec-child-before", &value);
    u2023202316_exec((void *)u2023202316_exec_target, INITPRIO + 1,
                     "lab4-exec", 2, 2023, 16);
    u2023202316_printf("[lab4 exec] exec returned unexpectedly\n");
    return SYSERR;
  }
  if (pid < 0) {
    u2023202316_printf("[lab4 exec] fork failed\n");
    return SYSERR;
  }
  u2023202316_print_ctx("exec-parent", &value);
  do {
    msg = u2023202316_receive();
  } while (msg != (umsg32)pid);
  return OK;
}

local process u2023202316_exec_target(int32 a, int32 b) {
  int32 value;

  value = a + b;
  u2023202316_print_ctx("exec-target", &value);
  u2023202316_printf("[lab4 exec-target] args=(%d,%d)\n", a, b);
  return OK;
}

local status u2023202316_heap_test(void) {
  char *a;
  char *b;

  a = (char *)u2023202316_umalloc(5000);
  b = (char *)u2023202316_umalloc(9000);
  u2023202316_printf("[lab4 heap] a=0x%08X b=0x%08X\n", a, b);
  if ((int32)a == SYSERR || (int32)b == SYSERR) {
    return SYSERR;
  }
  a[0] = 'A';
  a[4096] = 'B';
  b[0] = 'C';
  b[8192] = 'D';
  u2023202316_printf("[lab4 heap] values=%c %c %c %c\n", a[0], a[4096],
                     b[0], b[8192]);
  u2023202316_ufree(a);
  u2023202316_printf("[lab4 heap] freed a, leave b for exit cleanup\n");
  return OK;
}

local status u2023202316_stack_test(void) {
  u2023202316_printf("[lab4 stack] start stack growth test\n");
  u2023202316_stack_touch(6);
  u2023202316_printf("[lab4 stack] done stack growth test\n");
  return OK;
}

local void u2023202316_stack_touch(int32 depth) {
  volatile char block[4096];
  int32 value;

  value = depth;
  block[0] = (char)depth;
  block[4095] = (char)(depth + 1);
  u2023202316_printf("[lab4 stack] depth=%d &local=0x%08X block=0x%08X\n",
                     depth, &value, block);
  if (depth > 0) {
    u2023202316_stack_touch(depth - 1);
  }
}

local void u2023202316_print_ctx(char *tag, int32 *addr) {
  pid32 pid;
  char pname[PNMLEN];

  pid = u2023202316_getpid();
  u2023202316_getpname(pid, pname, sizeof(pname));
  u2023202316_printf("[lab4 %s] pid=%d name=%s function=%s addr=0x%08X\n",
                     tag, pid, pname, tag, addr);
}
