/* Lab3.h - experiment 3 user mode support */

#ifndef LAB3_H
#define LAB3_H

#include <tss.h>

#define K2023202316_KCODE_SEL 0x08
#define K2023202316_KDATA_SEL 0x10
#define K2023202316_KSTACK_SEL 0x18
#define K2023202316_UCODE_SEL 0x23
#define K2023202316_UDATA_SEL 0x2B
#define K2023202316_TSS_SEL 0x30

#define K2023202316_KERNEL_STK 4096
#define K2023202316_USER_STK 8192
#define K2023202316_MAX_UARGS 5
#define K2023202316_SYSCALL_VEC 0x80

#define K2023202316_SYS_GETPID 1
#define K2023202316_SYS_PUTC 2
#define K2023202316_SYS_SLEEPMS 3
#define K2023202316_SYS_EXIT 4
#define K2023202316_SYS_CREATE_USER_PROC 5
#define K2023202316_SYS_RESUME 6
#define K2023202316_SYS_RECEIVE 7

struct k2023202316_trapframe; // Lab4 2023202316

extern struct taskstate k2023202316_tss;

extern void k2023202316_init(void);
extern void k2023202316_set_tss_esp0(pid32);
extern pid32 k2023202316_create_user_proc(void *, uint32, pri16, char *, uint32,
                                          ...);
extern int32 k2023202316_syscall_dispatch(struct k2023202316_trapframe *);
extern void k2023202316_ltr(void);
extern void k2023202316_iret_to_user(void *, void *);
extern void k2023202316_syscall_entry(void);

extern int32 u2023202316_syscall(uint32, uint32, uint32, uint32, uint32,
                                 uint32);
extern pid32 u2023202316_getpid(void);
extern syscall u2023202316_putc(did32, char);
extern int32 u2023202316_printf(const char *, ...);
extern syscall u2023202316_sleepms(int32);
extern void u2023202316_exit(void);
extern pid32 u2023202316_create_user_proc(void *, uint32, pri16, char *, uint32,
                                          ...);
extern pri16 u2023202316_resume(pid32);
extern umsg32 u2023202316_receive(void);
extern uint32 u2023202316_getcpl(void);

#endif
