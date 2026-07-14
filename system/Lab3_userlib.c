/* Lab3_userlib.c - experiment 3 user mode library */

#include <xinu.h>
#include <stdarg.h>
#include <stdio.h>

extern void _fdoprnt(char *, va_list, int (*)(int, int), int);

local int u2023202316_print_putc(int, int);

/*------------------------------------------------------------------------
 * u2023202316_syscall - invoke the Lab3 syscall interrupt
 *------------------------------------------------------------------------
 */
int32 u2023202316_syscall(uint32 sysno, uint32 a1, uint32 a2, uint32 a3,
                          uint32 a4, uint32 a5) {
  int32 retval;

  asm volatile("int $0x80"
               : "=a"(retval)
               : "a"(sysno), "b"(a1), "c"(a2), "d"(a3), "S"(a4), "D"(a5)
               : "memory");
  return retval;
}

pid32 u2023202316_getpid(void) {
  return u2023202316_syscall(K2023202316_SYS_GETPID, 0, 0, 0, 0, 0);
}

syscall u2023202316_putc(did32 descrp, char ch) {
  return u2023202316_syscall(K2023202316_SYS_PUTC, (uint32)descrp, (uint32)ch,
                             0, 0, 0);
}

int32 u2023202316_printf(const char *fmt, ...) {
  va_list ap;

  va_start(ap, fmt);
  _fdoprnt((char *)fmt, ap, u2023202316_print_putc, CONSOLE);
  va_end(ap);
  return OK;
}

syscall u2023202316_sleepms(int32 delay) {
  return u2023202316_syscall(K2023202316_SYS_SLEEPMS, (uint32)delay, 0, 0, 0,
                             0);
}

void u2023202316_exit(void) {
  u2023202316_syscall(K2023202316_SYS_EXIT, 0, 0, 0, 0, 0);
  return;
}

pid32 u2023202316_create_user_proc(void *funcaddr, uint32 ssize,
                                   pri16 priority, char *name, uint32 nargs,
                                   ...) {
  va_list ap;
  uint32 argv[K2023202316_MAX_UARGS + 1];
  uint32 i;

  if (nargs > K2023202316_MAX_UARGS) {
    return SYSERR;
  }

  argv[0] = nargs;
  va_start(ap, nargs);
  for (i = 1; i <= nargs; i++) {
    argv[i] = va_arg(ap, uint32);
  }
  va_end(ap);

  return u2023202316_syscall(K2023202316_SYS_CREATE_USER_PROC,
                             (uint32)funcaddr, ssize, (uint32)priority,
                             (uint32)name, (uint32)argv);
}

pri16 u2023202316_resume(pid32 pid) {
  return u2023202316_syscall(K2023202316_SYS_RESUME, (uint32)pid, 0, 0, 0, 0);
}

umsg32 u2023202316_receive(void) {
  return u2023202316_syscall(K2023202316_SYS_RECEIVE, 0, 0, 0, 0, 0);
}

pid32 u2023202316_fork(void) {
  return u2023202316_syscall(K2023202316_SYS_FORK, 0, 0, 0, 0, 0);
}

void u2023202316_exec(void *funcaddr, pri16 priority, char *name, uint32 nargs,
                      ...) {
  va_list ap;
  uint32 argv[K2023202316_MAX_UARGS + 1];
  uint32 i;

  if (nargs > K2023202316_MAX_UARGS) {
    return;
  }
  argv[0] = nargs;
  va_start(ap, nargs);
  for (i = 1; i <= nargs; i++) {
    argv[i] = va_arg(ap, uint32);
  }
  va_end(ap);
  u2023202316_syscall(K2023202316_SYS_EXEC, (uint32)funcaddr,
                      (uint32)priority, (uint32)name, 0, (uint32)argv);
}

void *u2023202316_umalloc(uint32 nbytes) {
  return (void *)u2023202316_syscall(K2023202316_SYS_UMALLOC, nbytes, 0, 0, 0,
                                     0);
}

syscall u2023202316_ufree(void *ptr) {
  return u2023202316_syscall(K2023202316_SYS_UFREE, (uint32)ptr, 0, 0, 0, 0);
}

syscall u2023202316_getpname(pid32 pid, char *buf, uint32 len) {
  return u2023202316_syscall(K2023202316_SYS_GETPNAME, (uint32)pid,
                             (uint32)buf, len, 0, 0);
}

/*Lab6 2023202316: Begin*/
syscall u2023202316_writefile(char *name, char *buf, uint32 len) {
  return u2023202316_syscall(K2023202316_SYS_WRITEFILE, (uint32)name,
                             (uint32)buf, len, 0, 0);
}

syscall u2023202316_execfile(char *name, uint32 argc, char **argv) {
  return u2023202316_syscall(K2023202316_SYS_EXEC, 0, argc, (uint32)name,
                             (uint32)argv, 0);
}
/*Lab6 2023202316: End*/

uint32 u2023202316_getcpl(void) {
  uint16 cs;

  asm volatile("movw %%cs,%0" : "=r"(cs));
  return cs & 0x3;
}

local int u2023202316_print_putc(int dev, int ch) {
  return u2023202316_putc((did32)dev, (char)ch);
}
