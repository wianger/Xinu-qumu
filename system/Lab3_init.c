/* Lab3_init.c - experiment 3 TSS and syscall gate initialization */

#include <xinu.h>

struct __attribute__((__packed__)) k2023202316_sd {
  unsigned short sd_lolimit;
  unsigned short sd_lobase;
  unsigned char sd_midbase;
  unsigned char sd_access;
  unsigned char sd_hilim_fl;
  unsigned char sd_hibase;
};

struct __attribute__((__packed__)) k2023202316_idt {
  unsigned short igd_loffset;
  unsigned short igd_segsel;
  unsigned int igd_rsvd : 5;
  unsigned int igd_mbz : 3;
  unsigned int igd_type : 5;
  unsigned int igd_dpl : 2;
  unsigned int igd_present : 1;
  unsigned short igd_hoffset;
};

struct taskstate k2023202316_tss;

extern struct k2023202316_sd gdt[];
extern struct k2023202316_idt idt[];

local void k2023202316_set_tss_desc(void);
local void k2023202316_set_syscall_gate(void);

/*------------------------------------------------------------------------
 * k2023202316_init - initialize TSS and the user-callable syscall gate
 *------------------------------------------------------------------------
 */
void k2023202316_init(void) {
  memset(&k2023202316_tss, 0, sizeof(k2023202316_tss));
  k2023202316_tss.ss0 = K2023202316_KSTACK_SEL;
  k2023202316_tss.iomb = sizeof(struct taskstate);
  k2023202316_set_tss_esp0(currpid);
  k2023202316_set_tss_desc();
  k2023202316_ltr();
  k2023202316_set_syscall_gate();
}

/*------------------------------------------------------------------------
 * k2023202316_set_tss_esp0 - point ring transitions at pid's kernel stack
 *------------------------------------------------------------------------
 */
void k2023202316_set_tss_esp0(pid32 pid) {
  if ((pid >= 0) && (pid < NPROC) && (proctab[pid].prstkbase != NULL)) {
    k2023202316_tss.esp0 = (uint32)proctab[pid].prstkbase;
  }
}

local void k2023202316_set_tss_desc(void) {
  struct k2023202316_sd *psd;
  uint32 base;
  uint32 limit;

  psd = &gdt[6];
  base = (uint32)&k2023202316_tss;
  limit = sizeof(struct taskstate) - 1;

  psd->sd_lolimit = limit & 0xffff;
  psd->sd_lobase = base & 0xffff;
  psd->sd_midbase = (base >> 16) & 0xff;
  psd->sd_access = 0x89;
  psd->sd_hilim_fl = (limit >> 16) & 0x0f;
  psd->sd_hibase = (base >> 24) & 0xff;
}

local void k2023202316_set_syscall_gate(void) {
  struct k2023202316_idt *pidt;
  uint32 handler;

  handler = (uint32)k2023202316_syscall_entry;
  pidt = &idt[K2023202316_SYSCALL_VEC];
  pidt->igd_loffset = handler & 0xffff;
  pidt->igd_segsel = K2023202316_KCODE_SEL;
  pidt->igd_rsvd = 0;
  pidt->igd_mbz = 0;
  pidt->igd_type = 0x0e;
  pidt->igd_dpl = 3;
  pidt->igd_present = 1;
  pidt->igd_hoffset = handler >> 16;
}
