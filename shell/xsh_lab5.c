/* xsh_lab5.c - xsh_lab5 */

#include <xinu.h>
#include <stdio.h>

#define K2023202316_NAME "wangyihang"
#define K2023202316_ID "2023202316"

local void u2023202316_lab5_long_line(void);
local void u2023202316_lab5_scroll(void);
local void u2023202316_lab5_chars(void);

/*------------------------------------------------------------------------
 * xsh_lab5 - run experiment 5 VGA and keyboard console tests
 *------------------------------------------------------------------------
 */
shellcmd xsh_lab5(int nargs, char *args[]) {
  int32 x;
  int32 i;
  pid32 pid;
  char pname[PNMLEN];

  x = 2023202316;
  pid = u2023202316_getpid();
  u2023202316_getpname(pid, pname, sizeof(pname));

  u2023202316_printf("xsh_lab5: pid=%d name=%s cpl=%d &x=0x%08X\n", pid,
                     pname, u2023202316_getcpl(), &x);
  for (i = 0; i < nargs; i++) {
    u2023202316_printf("arg-%d: %s\n", i, args[i]);
  }
  u2023202316_printf("%s %s\n", K2023202316_ID, K2023202316_NAME);

  u2023202316_lab5_long_line();
  u2023202316_lab5_scroll();
  u2023202316_lab5_chars();
  u2023202316_printf("lab5 done\n");

  return SHELL_OK;
}

local void u2023202316_lab5_long_line(void) {
  int32 i;

  u2023202316_printf("[lab5] long line begin: ");
  for (i = 0; i < 96; i++) {
    u2023202316_printf("%c", 'A' + (i % 26));
  }
  u2023202316_printf("\n");
}

local void u2023202316_lab5_scroll(void) {
  int32 i;

  u2023202316_printf("[lab5] scrolling test begin\n");
  for (i = 0; i < 30; i++) {
    u2023202316_printf("scroll-line-%02d 0123456789 abcdefghijklmnopqrstuvwxyz "
                       "ABCDEFGHIJKLMNOPQRSTUVWXYZ\n",
                       i);
  }
}

local void u2023202316_lab5_chars(void) {
  u2023202316_printf("[lab5] chars: 0123456789 abc XYZ "
                     "!@#$%%^&*()_+-=[]{};:'\",.<>/?\\|\n");
  u2023202316_printf("[lab5] special: CR\rCR-after TAB\tTAB-after LF\n");
  u2023202316_printf("[lab5] backspace output: ABC\b \bD\n");
}
