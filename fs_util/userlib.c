#include "userlib.h"

#define CONSOLE 0
#define K2023202316_SYS_PUTC 2
#define K2023202316_SYS_EXIT 4
#define K2023202316_SYS_WRITEFILE 13

static void u2023202316_print_string(char *);
static void u2023202316_print_uint(uint32, uint32, int32);

int32 u2023202316_syscall(uint32 sysno, uint32 a1, uint32 a2, uint32 a3,
                          uint32 a4, uint32 a5) {
  int32 retval;

  asm volatile("int $0x80"
               : "=a"(retval)
               : "a"(sysno), "b"(a1), "c"(a2), "d"(a3), "S"(a4), "D"(a5)
               : "memory");
  return retval;
}

int32 u2023202316_putc(int32 dev, char ch) {
  return u2023202316_syscall(K2023202316_SYS_PUTC, (uint32)dev, (uint32)ch, 0,
                             0, 0);
}

int32 u2023202316_printf(const char *fmt, ...) {
  uint32 *ap;
  char ch;
  int32 width;
  int32 padzero;

  ap = (uint32 *)(&fmt + 1);
  while ((ch = *fmt++) != 0) {
    if (ch != '%') {
      u2023202316_putc(CONSOLE, ch);
      continue;
    }
    padzero = 0;
    width = 0;
    ch = *fmt++;
    if (ch == '0') {
      padzero = 1;
      ch = *fmt++;
    }
    while (ch >= '0' && ch <= '9') {
      width = width * 10 + ch - '0';
      ch = *fmt++;
    }
    if (ch == 's') {
      u2023202316_print_string((char *)*ap++);
    } else if (ch == 'd') {
      u2023202316_print_uint(*ap++, 10, width);
    } else if (ch == 'x' || ch == 'X') {
      u2023202316_print_uint(*ap++, 16, width);
    } else if (ch == 'c') {
      u2023202316_putc(CONSOLE, (char)*ap++);
    } else if (ch == '%') {
      u2023202316_putc(CONSOLE, '%');
    } else {
      if (padzero) {
        u2023202316_putc(CONSOLE, '0');
      }
      u2023202316_putc(CONSOLE, ch);
    }
  }
  return 0;
}

void u2023202316_exit(void) {
  u2023202316_syscall(K2023202316_SYS_EXIT, 0, 0, 0, 0, 0);
}

int32 u2023202316_writefile(char *name, char *buf, uint32 len) {
  return u2023202316_syscall(K2023202316_SYS_WRITEFILE, (uint32)name,
                             (uint32)buf, len, 0, 0);
}

static void u2023202316_print_string(char *s) {
  if (s == 0) {
    s = "(null)";
  }
  while (*s != 0) {
    u2023202316_putc(CONSOLE, *s++);
  }
}

static void u2023202316_print_uint(uint32 value, uint32 base, int32 width) {
  char buf[16];
  char *digits;
  int32 i;
  int32 n;

  digits = "0123456789ABCDEF";
  i = 0;
  do {
    buf[i++] = digits[value % base];
    value /= base;
  } while (value != 0 && i < (int32)sizeof(buf));
  for (n = i; n < width; n++) {
    u2023202316_putc(CONSOLE, '0');
  }
  while (i > 0) {
    u2023202316_putc(CONSOLE, buf[--i]);
  }
}
