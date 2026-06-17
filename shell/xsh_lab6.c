/* xsh_lab6.c - xsh_lab6_ls, xsh_lab6_run */

#include <xinu.h>
#include <stdio.h>

/*------------------------------------------------------------------------
 * xsh_lab6_ls - list files in the Lab6 disk image
 *------------------------------------------------------------------------
 */
shellcmd xsh_lab6_ls(int32 nargs, char *args[]) {
  (void)args;
  if (nargs != 1) {
    printf("usage: ls\n");
    return SHELL_ERROR;
  }
  return (k2023202316_fs_list() == OK) ? SHELL_OK : SHELL_ERROR;
}

/*------------------------------------------------------------------------
 * xsh_lab6_run - fork and exec an external ELF from the disk image
 *------------------------------------------------------------------------
 */
shellcmd xsh_lab6_run(int32 nargs, char *args[]) {
  pid32 pid;
  umsg32 msg;

  if (nargs < 2) {
    u2023202316_printf("usage: run program [args...]\n");
    return SHELL_ERROR;
  }

  pid = u2023202316_fork();
  if (pid == SYSERR) {
    u2023202316_printf("run: fork failed\n");
    return SHELL_ERROR;
  }
  if (pid == 0) {
    if (u2023202316_execfile(args[1], (uint32)(nargs - 1), &args[1]) ==
        SYSERR) {
      u2023202316_printf("run: cannot exec %s\n", args[1]);
    }
    u2023202316_exit();
    return SHELL_ERROR;
  }

  do {
    msg = u2023202316_receive();
  } while (msg != (umsg32)pid);
  return SHELL_OK;
}
