/* Lab4.h - experiment 4 paging and user address spaces */

#ifndef LAB4_H
#define LAB4_H

#define K2023202316_PAGE_SIZE 4096
#define K2023202316_PAGE_MASK 0xfffff000
#define K2023202316_PAGE_OFFSET 0x00000fff

#define K2023202316_PTE_P 0x001
#define K2023202316_PTE_W 0x002
#define K2023202316_PTE_U 0x004

#define K2023202316_KERNEL_IDENT_LIMIT 0x01000000
#define K2023202316_HIGH_FRAME_BASE K2023202316_KERNEL_IDENT_LIMIT
#define K2023202316_KMAP_BASE 0xffc00000
#define K2023202316_KMAP_SLOTS 4

#define K2023202316_UHEAP_BASE 0x40000000
#define K2023202316_UHEAP_LIMIT 0x40400000
#define K2023202316_USTACK_TOP 0x80000000
#define K2023202316_USTACK_MAX 0x00400000

#define K2023202316_SYS_FORK 8
#define K2023202316_SYS_EXEC 9
#define K2023202316_SYS_UMALLOC 10
#define K2023202316_SYS_UFREE 11
#define K2023202316_SYS_GETPNAME 12

#define K2023202316_MAX_HIGH_FRAMES 32768
#define K2023202316_VM_TRACE 1 // Lab4 2023202316

struct k2023202316_trapframe {
  uint32 edi;
  uint32 esi;
  uint32 ebp;
  uint32 esp_dummy;
  uint32 ebx;
  uint32 edx;
  uint32 ecx;
  uint32 eax;
  uint32 eip;
  uint32 cs;
  uint32 eflags;
  uint32 useresp;
  uint32 ss;
};

extern uint32 *k2023202316_kernel_pd;
extern uint32 k2023202316_kernel_pd_phys;

extern void k2023202316_vm_init(void);
extern void k2023202316_init_proc_vm_fields(struct procent *);
extern void k2023202316_switch_addrspace(pid32);
extern uint32 k2023202316_alloc_frame(char *, pid32, uint32);
extern void k2023202316_free_frame(uint32, char *, pid32, uint32);
extern syscall k2023202316_map_page(uint32, uint32, uint32, uint32);
extern uint32 k2023202316_lookup_page(uint32, uint32);
extern syscall k2023202316_copy_to_user(pid32, uint32, void *, uint32);
extern syscall k2023202316_copy_from_user(pid32, void *, uint32, uint32);
extern uint32 k2023202316_create_addrspace(pid32);
extern syscall k2023202316_clone_addrspace(pid32, pid32);
extern syscall k2023202316_reset_user_space(pid32);
extern syscall k2023202316_free_user_space(pid32);
extern syscall k2023202316_map_user_stack(pid32, uint32);
extern uint32 k2023202316_build_user_stack(pid32, void *, uint32, uint32 *);
extern syscall k2023202316_map_user_region(pid32, uint32, uint32,
                                           char *); // Lab6 2023202316
extern pid32 k2023202316_lab4_newpid(void);
extern pid32 k2023202316_fork_from_trapframe(struct k2023202316_trapframe *);
extern syscall k2023202316_exec_from_trapframe(struct k2023202316_trapframe *,
                                               void *, pri16, char *, uint32,
                                               uint32 *);
extern void *k2023202316_user_malloc(uint32);
extern syscall k2023202316_user_free(void *);
extern syscall k2023202316_getpname(pid32, char *, uint32);
extern int32 k2023202316_page_fault_handler(struct k2023202316_trapframe *,
                                            uint32);

extern void k2023202316_load_cr3(uint32);
extern uint32 k2023202316_read_cr0(void);
extern void k2023202316_write_cr0(uint32);
extern uint32 k2023202316_read_cr2(void);
extern void k2023202316_invlpg(void *);
extern void k2023202316_restore_trapframe(struct k2023202316_trapframe *);
extern void k2023202316_page_fault_entry(void);

extern pid32 u2023202316_fork(void);
extern void u2023202316_exec(void *, pri16, char *, uint32, ...);
extern void *u2023202316_umalloc(uint32);
extern syscall u2023202316_ufree(void *);
extern syscall u2023202316_getpname(pid32, char *, uint32);

#endif
