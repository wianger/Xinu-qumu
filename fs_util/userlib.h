#ifndef USERLIB_H
#define USERLIB_H

typedef unsigned int uint32;
typedef int int32;

int32 u2023202316_syscall(uint32, uint32, uint32, uint32, uint32, uint32);
int32 u2023202316_putc(int32, char);
int32 u2023202316_printf(const char *, ...);
void u2023202316_exit(void);
int32 u2023202316_writefile(char *, char *, uint32);

#endif
