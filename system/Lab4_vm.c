/* Lab4_vm.c - experiment 4 paging and user address spaces */

#include <xinu.h>

struct k2023202316_frame {
  uint32 phys;
  bool8 used;
  char purpose[24];
  pid32 pid;
  uint32 vaddr;
};

uint32 *k2023202316_kernel_pd;
uint32 k2023202316_kernel_pd_phys;
extern struct mbootinfo *bootinfo;

local struct k2023202316_frame k2023202316_frames[K2023202316_MAX_HIGH_FRAMES];
local uint32 k2023202316_nframes;

local void k2023202316_add_high_region(uint32, uint32);
local uint32 k2023202316_round_page(uint32);
local uint32 k2023202316_trunc_page(uint32);
local uint32 *k2023202316_kmap(uint32, uint32);
local void k2023202316_kunmap(uint32);
local void k2023202316_zero_frame(uint32);
local void k2023202316_copy_frame(uint32, uint32);
local void k2023202316_copy_purpose(char *, char *);
local uint32 *k2023202316_pd_from_phys(uint32);
local uint32 *k2023202316_pt_from_phys(uint32);
local uint32 k2023202316_ensure_pt(uint32, uint32, uint32);
local uint32 k2023202316_lookup_page_raw(uint32, uint32);
local syscall k2023202316_unmap_page(pid32, uint32, bool8, char *);
local syscall k2023202316_copy_bytes(pid32, uint32, uint32, void *, uint32,
                                     bool8);
local pid32 k2023202316_pid_for_pd(uint32);
local void k2023202316_set_name(struct procent *, char *);
local void k2023202316_copy_desc(pid32, pid32);
local char *k2023202316_addr_to_purpose(pid32, uint32);
local uint32 k2023202316_alloc_low_page(char *, pid32, uint32);

/*------------------------------------------------------------------------
 * k2023202316_vm_init - initialize paging and high-memory frame allocator
 *------------------------------------------------------------------------
 */
void k2023202316_vm_init(void) {
  struct mbmregion *mmap_addr;
  struct mbmregion *mmap_addrend;
  uint32 pd_phys;
  uint32 pt_phys;
  uint32 *pd;
  uint32 *pt;
  uint32 phys;
  uint32 flags;

  k2023202316_nframes = 0;
  mmap_addr = (struct mbmregion *)bootinfo->mmap_addr;
  mmap_addrend =
      (struct mbmregion *)((uint8 *)mmap_addr + bootinfo->mmap_length);

  while (mmap_addr < mmap_addrend) {
    if (mmap_addr->type == MULTIBOOT_MMAP_TYPE_USABLE) {
      k2023202316_add_high_region((uint32)mmap_addr->base_addr,
                                  (uint32)mmap_addr->length);
    }
    mmap_addr = (struct mbmregion *)((uint8 *)mmap_addr + mmap_addr->size + 4);
  }

  pd_phys = k2023202316_alloc_low_page("kernel-pd", NULLPROC, 0);
  if (pd_phys == 0) {
    panic("Lab4 cannot allocate kernel page directory");
  }
  k2023202316_kernel_pd_phys = pd_phys;
  k2023202316_kernel_pd = (uint32 *)pd_phys;
  memset(k2023202316_kernel_pd, 0, K2023202316_PAGE_SIZE);
  pd = k2023202316_pd_from_phys(pd_phys);

  for (phys = 0; phys < K2023202316_KERNEL_IDENT_LIMIT;
       phys += K2023202316_PAGE_SIZE) {
    if ((phys & 0x003fffff) == 0) {
      pt_phys = k2023202316_alloc_low_page("kernel-pt", NULLPROC, phys);
      if (pt_phys == 0) {
        panic("Lab4 cannot allocate kernel page table");
      }
      memset((void *)pt_phys, 0, K2023202316_PAGE_SIZE);
      pd[phys >> 22] = pt_phys | K2023202316_PTE_P | K2023202316_PTE_W |
                        K2023202316_PTE_U;
    }
    pt = k2023202316_pt_from_phys(pd[phys >> 22] & K2023202316_PAGE_MASK);
    flags = K2023202316_PTE_P | K2023202316_PTE_W;
    if ((phys >= (uint32)&text) && (phys < (uint32)&etext)) {
      flags = K2023202316_PTE_P | K2023202316_PTE_U;
    }
    pt[(phys >> 12) & 0x3ff] = phys | flags;
  }

  pt_phys =
      k2023202316_alloc_low_page("kmap-pt", NULLPROC, K2023202316_KMAP_BASE);
  if (pt_phys == 0) {
    panic("Lab4 cannot allocate kmap page table");
  }
  memset((void *)pt_phys, 0, K2023202316_PAGE_SIZE);
  pd[K2023202316_KMAP_BASE >> 22] =
      pt_phys | K2023202316_PTE_P | K2023202316_PTE_W;

  k2023202316_load_cr3(pd_phys);
  k2023202316_write_cr0(k2023202316_read_cr0() | 0x80000000);
  kprintf("Lab4 2023202316: paging enabled, high frames=%u\n",
          k2023202316_nframes);
}

/*------------------------------------------------------------------------
 * k2023202316_init_proc_vm_fields - clear Lab4 fields in a process table row
 *------------------------------------------------------------------------
 */
void k2023202316_init_proc_vm_fields(struct procent *prptr) {
  int32 i;

  prptr->pr2023202316_pdbr = 0;
  prptr->pr2023202316_ustacktop = 0;
  prptr->pr2023202316_ustackbase = 0;
  prptr->pr2023202316_ustackmax = 0;
  prptr->pr2023202316_uheapnext = K2023202316_UHEAP_BASE;
  for (i = 0; i < K2023202316_MAX_UALLOCS; i++) {
    prptr->pr2023202316_uallocs[i].base = 0;
    prptr->pr2023202316_uallocs[i].npages = 0;
    prptr->pr2023202316_uallocs[i].used = FALSE;
  }
}

void k2023202316_switch_addrspace(pid32 pid) {
  uint32 pdbr;

  pdbr = 0;
  if ((pid >= 0) && (pid < NPROC)) {
    pdbr = proctab[pid].pr2023202316_pdbr;
  }
  if (pdbr == 0) {
    pdbr = k2023202316_kernel_pd_phys;
  }
  k2023202316_load_cr3(pdbr);
}

uint32 k2023202316_alloc_frame(char *purpose, pid32 pid, uint32 vaddr) {
  uint32 i;

  for (i = 0; i < k2023202316_nframes; i++) {
    if (!k2023202316_frames[i].used) {
      k2023202316_frames[i].used = TRUE;
      k2023202316_frames[i].pid = pid;
      k2023202316_frames[i].vaddr = vaddr;
      k2023202316_copy_purpose(k2023202316_frames[i].purpose, purpose);
      kprintf("Lab4 2023202316 alloc page phys=0x%08X no=%u pid=%d use=%s "
              "vaddr=0x%08X\n",
              k2023202316_frames[i].phys,
              k2023202316_frames[i].phys / K2023202316_PAGE_SIZE, pid,
              k2023202316_frames[i].purpose, vaddr);
      return k2023202316_frames[i].phys;
    }
  }
  kprintf("Lab4 2023202316 alloc page failed use=%s pid=%d vaddr=0x%08X\n",
          purpose, pid, vaddr);
  return 0;
}

void k2023202316_free_frame(uint32 phys, char *purpose, pid32 pid,
                            uint32 vaddr) {
  uint32 i;

  phys &= K2023202316_PAGE_MASK;
  if (phys == 0) {
    return;
  }
  for (i = 0; i < k2023202316_nframes; i++) {
    if (k2023202316_frames[i].phys == phys) {
      if (!k2023202316_frames[i].used) {
        return;
      }
      kprintf("Lab4 2023202316 free page phys=0x%08X no=%u pid=%d use=%s "
              "vaddr=0x%08X\n",
              phys, phys / K2023202316_PAGE_SIZE, pid, purpose, vaddr);
      k2023202316_frames[i].used = FALSE;
      k2023202316_frames[i].pid = SYSERR;
      k2023202316_frames[i].vaddr = 0;
      k2023202316_frames[i].purpose[0] = NULLCH;
      return;
    }
  }
}

local uint32 k2023202316_alloc_low_page(char *purpose, pid32 pid,
                                        uint32 vaddr) {
  char *mem;
  uint32 phys;

  mem = getmem(K2023202316_PAGE_SIZE);
  if (mem == (char *)SYSERR) {
    return 0;
  }
  phys = (uint32)mem;
  if (phys & K2023202316_PAGE_OFFSET) {
    freemem(mem, K2023202316_PAGE_SIZE);
    mem = getmem(K2023202316_PAGE_SIZE * 2);
    if (mem == (char *)SYSERR) {
      return 0;
    }
    phys = k2023202316_round_page((uint32)mem);
  }
  kprintf("Lab4 2023202316 alloc low page phys=0x%08X no=%u pid=%d use=%s "
          "vaddr=0x%08X\n",
          phys, phys / K2023202316_PAGE_SIZE, pid, purpose, vaddr);
  return phys;
}

syscall k2023202316_map_page(uint32 pd_phys, uint32 vaddr, uint32 frame,
                             uint32 flags) {
  uint32 *pt;
  uint32 pt_phys;

  vaddr &= K2023202316_PAGE_MASK;
  frame &= K2023202316_PAGE_MASK;
  pt_phys = k2023202316_ensure_pt(pd_phys, vaddr, flags);
  if (pt_phys == 0) {
    return SYSERR;
  }
  pt = k2023202316_pt_from_phys(pt_phys);
  pt[(vaddr >> 12) & 0x3ff] = frame | flags | K2023202316_PTE_P;
  k2023202316_invlpg((void *)vaddr);
  return OK;
}

uint32 k2023202316_lookup_page(uint32 pd_phys, uint32 vaddr) {
  return k2023202316_lookup_page_raw(pd_phys, vaddr);
}

uint32 k2023202316_create_addrspace(pid32 pid) {
  uint32 pd_phys;
  uint32 *pd;

  pd_phys = k2023202316_alloc_frame("user-pd", pid, 0);
  if (pd_phys == 0) {
    return 0;
  }
  k2023202316_copy_frame(k2023202316_kernel_pd_phys, pd_phys);
  pd = k2023202316_pd_from_phys(pd_phys);
  pd[K2023202316_UHEAP_BASE >> 22] = 0;
  pd[K2023202316_USTACK_TOP >> 22] = 0;
  pd[(K2023202316_USTACK_TOP - 1) >> 22] = 0;
  proctab[pid].pr2023202316_pdbr = pd_phys;
  proctab[pid].pr2023202316_ustacktop = K2023202316_USTACK_TOP;
  proctab[pid].pr2023202316_ustackbase = K2023202316_USTACK_TOP;
  proctab[pid].pr2023202316_ustackmax =
      K2023202316_USTACK_TOP - K2023202316_USTACK_MAX;
  proctab[pid].pr2023202316_uheapnext = K2023202316_UHEAP_BASE;
  return pd_phys;
}

syscall k2023202316_map_user_stack(pid32 pid, uint32 nbytes) {
  struct procent *prptr;
  uint32 npages;
  uint32 i;
  uint32 vaddr;
  uint32 frame;

  if (isbadpid(pid)) {
    return SYSERR;
  }
  prptr = &proctab[pid];
  npages = (k2023202316_round_page(nbytes)) / K2023202316_PAGE_SIZE;
  if (npages == 0) {
    npages = 1;
  }
  if (npages * K2023202316_PAGE_SIZE > K2023202316_USTACK_MAX) {
    return SYSERR;
  }
  for (i = 0; i < npages; i++) {
    vaddr = K2023202316_USTACK_TOP - ((i + 1) * K2023202316_PAGE_SIZE);
    frame = k2023202316_alloc_frame("user-stack", pid, vaddr);
    if (frame == 0) {
      return SYSERR;
    }
    k2023202316_zero_frame(frame);
    if (k2023202316_map_page(prptr->pr2023202316_pdbr, vaddr, frame,
                             K2023202316_PTE_P | K2023202316_PTE_W |
                                 K2023202316_PTE_U) == SYSERR) {
      k2023202316_free_frame(frame, "user-stack", pid, vaddr);
      return SYSERR;
    }
    if (vaddr < prptr->pr2023202316_ustackbase) {
      prptr->pr2023202316_ustackbase = vaddr;
    }
  }
  prptr->pr2023202316_ustkbase = (char *)K2023202316_USTACK_TOP;
  prptr->pr2023202316_ustklen =
      K2023202316_USTACK_TOP - prptr->pr2023202316_ustackbase;
  return OK;
}

uint32 k2023202316_build_user_stack(pid32 pid, void *retaddr, uint32 nargs,
                                    uint32 *args) {
  uint32 sp;
  uint32 value;
  uint32 i;

  if (nargs > K2023202316_MAX_UARGS) {
    return 0;
  }
  sp = K2023202316_USTACK_TOP;
  value = STACKMAGIC;
  sp -= sizeof(uint32);
  if (k2023202316_copy_to_user(pid, sp, &value, sizeof(uint32)) == SYSERR) {
    return 0;
  }
  for (i = nargs; i > 0; i--) {
    value = args[i - 1];
    sp -= sizeof(uint32);
    if (k2023202316_copy_to_user(pid, sp, &value, sizeof(uint32)) == SYSERR) {
      return 0;
    }
  }
  value = (uint32)retaddr;
  sp -= sizeof(uint32);
  if (k2023202316_copy_to_user(pid, sp, &value, sizeof(uint32)) == SYSERR) {
    return 0;
  }
  proctab[pid].pr2023202316_ustkptr = (char *)sp;
  return sp;
}

syscall k2023202316_copy_to_user(pid32 pid, uint32 dst, void *src,
                                 uint32 nbytes) {
  return k2023202316_copy_bytes(pid, dst, 0, src, nbytes, TRUE);
}

syscall k2023202316_copy_from_user(pid32 pid, void *dst, uint32 src,
                                   uint32 nbytes) {
  return k2023202316_copy_bytes(pid, 0, src, dst, nbytes, FALSE);
}

pid32 k2023202316_lab4_newpid(void) {
  uint32 i;
  static pid32 nextpid = 1;

  for (i = 0; i < NPROC; i++) {
    nextpid %= NPROC;
    if (proctab[nextpid].prstate == PR_FREE) {
      return nextpid++;
    }
    nextpid++;
  }
  return (pid32)SYSERR;
}

syscall k2023202316_clone_addrspace(pid32 parentpid, pid32 childpid) {
  struct procent *parent;
  struct procent *child;
  uint32 vaddr;
  uint32 pte;
  uint32 frame;
  uint32 newframe;
  int32 i;

  parent = &proctab[parentpid];
  child = &proctab[childpid];
  if (k2023202316_create_addrspace(childpid) == 0) {
    return SYSERR;
  }

  for (vaddr = parent->pr2023202316_ustackbase;
       vaddr < parent->pr2023202316_ustacktop; vaddr += K2023202316_PAGE_SIZE) {
    pte = k2023202316_lookup_page(parent->pr2023202316_pdbr, vaddr);
    if (!(pte & K2023202316_PTE_P)) {
      continue;
    }
    frame = pte & K2023202316_PAGE_MASK;
    newframe = k2023202316_alloc_frame("fork-stack", childpid, vaddr);
    if (newframe == 0) {
      return SYSERR;
    }
    k2023202316_copy_frame(frame, newframe);
    if (k2023202316_map_page(child->pr2023202316_pdbr, vaddr, newframe,
                             K2023202316_PTE_P | K2023202316_PTE_W |
                                 K2023202316_PTE_U) == SYSERR) {
      return SYSERR;
    }
  }
  child->pr2023202316_ustacktop = parent->pr2023202316_ustacktop;
  child->pr2023202316_ustackbase = parent->pr2023202316_ustackbase;
  child->pr2023202316_ustackmax = parent->pr2023202316_ustackmax;
  child->pr2023202316_ustkbase = parent->pr2023202316_ustkbase;
  child->pr2023202316_ustkptr = parent->pr2023202316_ustkptr;
  child->pr2023202316_ustklen = parent->pr2023202316_ustklen;
  child->pr2023202316_uheapnext = parent->pr2023202316_uheapnext;

  for (i = 0; i < K2023202316_MAX_UALLOCS; i++) {
    child->pr2023202316_uallocs[i] = parent->pr2023202316_uallocs[i];
    if (!parent->pr2023202316_uallocs[i].used) {
      continue;
    }
    for (vaddr = parent->pr2023202316_uallocs[i].base;
         vaddr < parent->pr2023202316_uallocs[i].base +
                     parent->pr2023202316_uallocs[i].npages *
                         K2023202316_PAGE_SIZE;
         vaddr += K2023202316_PAGE_SIZE) {
      pte = k2023202316_lookup_page(parent->pr2023202316_pdbr, vaddr);
      if (!(pte & K2023202316_PTE_P)) {
        return SYSERR;
      }
      newframe = k2023202316_alloc_frame("fork-heap", childpid, vaddr);
      if (newframe == 0) {
        return SYSERR;
      }
      k2023202316_copy_frame(pte & K2023202316_PAGE_MASK, newframe);
      if (k2023202316_map_page(child->pr2023202316_pdbr, vaddr, newframe,
                               K2023202316_PTE_P | K2023202316_PTE_W |
                                   K2023202316_PTE_U) == SYSERR) {
        return SYSERR;
      }
    }
  }
  return OK;
}

syscall k2023202316_reset_user_space(pid32 pid) {
  if (k2023202316_free_user_space(pid) == SYSERR) {
    return SYSERR;
  }
  k2023202316_init_proc_vm_fields(&proctab[pid]);
  if (k2023202316_create_addrspace(pid) == 0) {
    return SYSERR;
  }
  return k2023202316_map_user_stack(pid, K2023202316_USER_STK);
}

syscall k2023202316_free_user_space(pid32 pid) {
  struct procent *prptr;
  uint32 pd_phys;
  uint32 *pd;
  uint32 *pt;
  uint32 pdi;
  uint32 pti;
  uint32 vaddr;
  uint32 pde;
  uint32 pte;
  uint32 oldcr3;

  if ((pid < 0) || (pid >= NPROC)) {
    return SYSERR;
  }
  prptr = &proctab[pid];
  pd_phys = prptr->pr2023202316_pdbr;
  if (pd_phys == 0 || pd_phys == k2023202316_kernel_pd_phys) {
    return OK;
  }
  oldcr3 = proctab[currpid].pr2023202316_pdbr;
  if (oldcr3 == 0) {
    oldcr3 = k2023202316_kernel_pd_phys;
  }
  if (oldcr3 != k2023202316_kernel_pd_phys) {
    k2023202316_load_cr3(k2023202316_kernel_pd_phys);
  }

  pd = k2023202316_pd_from_phys(pd_phys);
  for (pdi = 0; pdi < 1024; pdi++) {
    pde = pd[pdi];
    if (!(pde & K2023202316_PTE_P)) {
      continue;
    }
    pt = k2023202316_pt_from_phys(pde & K2023202316_PAGE_MASK);
    for (pti = 0; pti < 1024; pti++) {
      pte = pt[pti];
      vaddr = (pdi << 22) | (pti << 12);
      if (!(pte & K2023202316_PTE_P)) {
        continue;
      }
      if (vaddr >= K2023202316_UHEAP_BASE &&
          vaddr < K2023202316_USTACK_TOP) {
        k2023202316_free_frame(pte & K2023202316_PAGE_MASK,
                               k2023202316_addr_to_purpose(pid, vaddr), pid,
                               vaddr);
        pt[pti] = 0;
      }
    }
    if ((pdi == (K2023202316_UHEAP_BASE >> 22)) ||
        (pdi == ((K2023202316_USTACK_TOP - 1) >> 22))) {
      k2023202316_free_frame(pde & K2023202316_PAGE_MASK, "user-pt", pid,
                             pdi << 22);
      pd[pdi] = 0;
    }
  }
  k2023202316_free_frame(pd_phys, "user-pd", pid, 0);
  prptr->pr2023202316_pdbr = 0;
  k2023202316_init_proc_vm_fields(prptr);
  if (pid != currpid && oldcr3 != k2023202316_kernel_pd_phys) {
    k2023202316_load_cr3(oldcr3);
  }
  return OK;
}

pid32 k2023202316_fork_from_trapframe(struct k2023202316_trapframe *tf) {
  intmask mask;
  pid32 childpid;
  struct procent *parent;
  struct procent *child;
  char *kstkbase;
  uint32 *saddr;
  uint32 savsp;
  uint32 *pushsp;
  struct k2023202316_trapframe *childtf;

  mask = disable();
  parent = &proctab[currpid];
  childpid = k2023202316_lab4_newpid();
  if (childpid == SYSERR) {
    restore(mask);
    return SYSERR;
  }
  kstkbase = getstk(K2023202316_KERNEL_STK);
  if (kstkbase == (char *)SYSERR) {
    restore(mask);
    return SYSERR;
  }

  prcount++;
  child = &proctab[childpid];
  child->prstate = PR_SUSP;
  child->prprio = parent->prprio;
  child->prstkbase = kstkbase;
  child->prstklen = K2023202316_KERNEL_STK;
  child->prsem = -1;
  child->prparent = currpid;
  child->prhasmsg = FALSE;
  child->prmsg = 0;
  child->pr2023202316_isuser = TRUE;
  k2023202316_init_proc_vm_fields(child);
  k2023202316_set_name(child, parent->prname);
  k2023202316_copy_desc(currpid, childpid);

  if (k2023202316_clone_addrspace(currpid, childpid) == SYSERR) {
    freestk(kstkbase, K2023202316_KERNEL_STK);
    child->prstate = PR_FREE;
    prcount--;
    restore(mask);
    return SYSERR;
  }

  saddr = (uint32 *)kstkbase;
  *saddr = STACKMAGIC;
  savsp = (uint32)saddr;
  saddr -= sizeof(struct k2023202316_trapframe) / sizeof(uint32);
  childtf = (struct k2023202316_trapframe *)saddr;
  *childtf = *tf;
  childtf->eax = 0;
  *--saddr = (uint32)childtf;
  *--saddr = 0;
  *--saddr = (uint32)k2023202316_restore_trapframe;
  *--saddr = savsp;
  savsp = (uint32)saddr;
  *--saddr = 0x00000200;
  *--saddr = 0;
  *--saddr = 0;
  *--saddr = 0;
  *--saddr = 0;
  *--saddr = 0;
  pushsp = saddr;
  *--saddr = savsp;
  *--saddr = 0;
  *--saddr = 0;
  *pushsp = (uint32)saddr;
  child->prstkptr = (char *)saddr;

  ready(childpid);
  restore(mask);
  return childpid;
}

syscall k2023202316_exec_from_trapframe(struct k2023202316_trapframe *tf,
                                        void *funcaddr, pri16 priority,
                                        char *name, uint32 nargs,
                                        uint32 *args) {
  uint32 user_args[K2023202316_MAX_UARGS];
  uint32 newsp;
  uint32 i;

  if ((funcaddr == NULL) || (priority < 1) ||
      (nargs > K2023202316_MAX_UARGS)) {
    return SYSERR;
  }
  for (i = 0; i < nargs; i++) {
    user_args[i] = args[i];
  }
  if (k2023202316_reset_user_space(currpid) == SYSERR) {
    return SYSERR;
  }
  newsp = k2023202316_build_user_stack(currpid, (void *)u2023202316_exit,
                                       nargs, user_args);
  if (newsp == 0) {
    return SYSERR;
  }
  k2023202316_set_name(&proctab[currpid], name);
  proctab[currpid].prprio = priority;
  k2023202316_switch_addrspace(currpid);
  tf->eip = (uint32)funcaddr;
  tf->useresp = newsp;
  tf->eax = 0;
  return OK;
}

void *k2023202316_user_malloc(uint32 nbytes) {
  struct procent *prptr;
  uint32 npages;
  uint32 base;
  uint32 frame;
  uint32 i;
  int32 slot;

  prptr = &proctab[currpid];
  if (!prptr->pr2023202316_isuser || nbytes == 0) {
    return (void *)SYSERR;
  }
  npages = k2023202316_round_page(nbytes) / K2023202316_PAGE_SIZE;
  base = prptr->pr2023202316_uheapnext;
  if (base + npages * K2023202316_PAGE_SIZE > K2023202316_UHEAP_LIMIT) {
    return (void *)SYSERR;
  }
  slot = -1;
  for (i = 0; i < K2023202316_MAX_UALLOCS; i++) {
    if (!prptr->pr2023202316_uallocs[i].used) {
      slot = i;
      break;
    }
  }
  if (slot < 0) {
    return (void *)SYSERR;
  }
  for (i = 0; i < npages; i++) {
    frame = k2023202316_alloc_frame("user-heap", currpid,
                                    base + i * K2023202316_PAGE_SIZE);
    if (frame == 0) {
      return (void *)SYSERR;
    }
    k2023202316_zero_frame(frame);
    if (k2023202316_map_page(prptr->pr2023202316_pdbr,
                             base + i * K2023202316_PAGE_SIZE, frame,
                             K2023202316_PTE_P | K2023202316_PTE_W |
                                 K2023202316_PTE_U) == SYSERR) {
      return (void *)SYSERR;
    }
  }
  prptr->pr2023202316_uallocs[slot].base = base;
  prptr->pr2023202316_uallocs[slot].npages = npages;
  prptr->pr2023202316_uallocs[slot].used = TRUE;
  prptr->pr2023202316_uheapnext += npages * K2023202316_PAGE_SIZE;
  return (void *)base;
}

syscall k2023202316_user_free(void *ptr) {
  struct procent *prptr;
  uint32 base;
  uint32 vaddr;
  uint32 i;
  uint32 j;

  prptr = &proctab[currpid];
  base = (uint32)ptr & K2023202316_PAGE_MASK;
  for (i = 0; i < K2023202316_MAX_UALLOCS; i++) {
    if (!prptr->pr2023202316_uallocs[i].used ||
        prptr->pr2023202316_uallocs[i].base != base) {
      continue;
    }
    for (j = 0; j < prptr->pr2023202316_uallocs[i].npages; j++) {
      vaddr = base + j * K2023202316_PAGE_SIZE;
      k2023202316_unmap_page(currpid, vaddr, TRUE, "user-heap");
    }
    prptr->pr2023202316_uallocs[i].used = FALSE;
    return OK;
  }
  return SYSERR;
}

syscall k2023202316_getpname(pid32 pid, char *buf, uint32 len) {
  char tmp[PNMLEN];
  uint32 n;

  if ((pid < 0) || (pid >= NPROC) || (buf == NULL) || (len == 0)) {
    return SYSERR;
  }
  for (n = 0; n < PNMLEN; n++) {
    tmp[n] = proctab[pid].prname[n];
    if (tmp[n] == NULLCH) {
      break;
    }
  }
  tmp[PNMLEN - 1] = NULLCH;
  if (len > PNMLEN) {
    len = PNMLEN;
  }
  return k2023202316_copy_to_user(currpid, (uint32)buf, tmp, len);
}

int32 k2023202316_page_fault_handler(struct k2023202316_trapframe *tf,
                                     uint32 err) {
  uint32 fault;
  uint32 vaddr;
  uint32 frame;
  uint32 cs;
  uint32 *raw;
  struct procent *prptr;

  fault = k2023202316_read_cr2();
  vaddr = fault & K2023202316_PAGE_MASK;
  prptr = &proctab[currpid];
  raw = (uint32 *)tf;
  cs = raw[10];
  if ((cs & 0x3) == 3 && prptr->pr2023202316_isuser &&
      !(err & K2023202316_PTE_P) && vaddr >= prptr->pr2023202316_ustackmax &&
      vaddr < prptr->pr2023202316_ustackbase) {
    frame = k2023202316_alloc_frame("stack-grow", currpid, vaddr);
    if (frame == 0) {
      return FALSE;
    }
    k2023202316_zero_frame(frame);
    if (k2023202316_map_page(prptr->pr2023202316_pdbr, vaddr, frame,
                             K2023202316_PTE_P | K2023202316_PTE_W |
                                 K2023202316_PTE_U) == SYSERR) {
      return FALSE;
    }
    prptr->pr2023202316_ustackbase = vaddr;
    prptr->pr2023202316_ustklen =
        prptr->pr2023202316_ustacktop - prptr->pr2023202316_ustackbase;
    kprintf("Lab4 2023202316 stack grow pid=%d vaddr=0x%08X\n", currpid,
            vaddr);
    return TRUE;
  }

  kprintf("Lab4 2023202316 page fault pid=%d name=%s addr=0x%08X err=0x%X "
          "cs=0x%X\n",
          currpid, prptr->prname, fault, err, cs);
  if ((cs & 0x3) == 3) {
    kill(currpid);
  }
  return FALSE;
}

local void k2023202316_add_high_region(uint32 base, uint32 length) {
  uint32 start;
  uint32 endaddr;

  if (length == 0) {
    return;
  }
  start = k2023202316_round_page(base);
  endaddr = k2023202316_trunc_page(base + length);
  if (start < K2023202316_HIGH_FRAME_BASE) {
    start = K2023202316_HIGH_FRAME_BASE;
  }
  while (start + K2023202316_PAGE_SIZE <= endaddr &&
         k2023202316_nframes < K2023202316_MAX_HIGH_FRAMES) {
    k2023202316_frames[k2023202316_nframes].phys = start;
    k2023202316_frames[k2023202316_nframes].used = FALSE;
    k2023202316_frames[k2023202316_nframes].pid = SYSERR;
    k2023202316_frames[k2023202316_nframes].vaddr = 0;
    k2023202316_frames[k2023202316_nframes].purpose[0] = NULLCH;
    k2023202316_nframes++;
    start += K2023202316_PAGE_SIZE;
  }
}

local uint32 k2023202316_round_page(uint32 value) {
  return (value + K2023202316_PAGE_SIZE - 1) & K2023202316_PAGE_MASK;
}

local uint32 k2023202316_trunc_page(uint32 value) {
  return value & K2023202316_PAGE_MASK;
}

local uint32 *k2023202316_kmap(uint32 phys, uint32 slot) {
  uint32 *pd;
  uint32 *pt;
  uint32 vaddr;

  pd = k2023202316_pd_from_phys(k2023202316_kernel_pd_phys);
  pt = k2023202316_pt_from_phys(pd[K2023202316_KMAP_BASE >> 22] &
                                K2023202316_PAGE_MASK);
  vaddr = K2023202316_KMAP_BASE + slot * K2023202316_PAGE_SIZE;
  pt[slot] = (phys & K2023202316_PAGE_MASK) | K2023202316_PTE_P |
             K2023202316_PTE_W;
  k2023202316_invlpg((void *)vaddr);
  return (uint32 *)vaddr;
}

local void k2023202316_kunmap(uint32 slot) {
  uint32 *pd;
  uint32 *pt;
  uint32 vaddr;

  pd = k2023202316_pd_from_phys(k2023202316_kernel_pd_phys);
  pt = k2023202316_pt_from_phys(pd[K2023202316_KMAP_BASE >> 22] &
                                K2023202316_PAGE_MASK);
  vaddr = K2023202316_KMAP_BASE + slot * K2023202316_PAGE_SIZE;
  pt[slot] = 0;
  k2023202316_invlpg((void *)vaddr);
}

local void k2023202316_zero_frame(uint32 phys) {
  uint32 *ptr;

  ptr = k2023202316_kmap(phys, 0);
  memset(ptr, 0, K2023202316_PAGE_SIZE);
  k2023202316_kunmap(0);
}

local void k2023202316_copy_frame(uint32 src, uint32 dst) {
  uint32 *sptr;
  uint32 *dptr;

  sptr = k2023202316_kmap(src, 0);
  dptr = k2023202316_kmap(dst, 1);
  memcpy(dptr, sptr, K2023202316_PAGE_SIZE);
  k2023202316_kunmap(1);
  k2023202316_kunmap(0);
}

local void k2023202316_copy_purpose(char *dst, char *src) {
  int32 i;

  if (src == NULL) {
    src = "page";
  }
  for (i = 0; i < 23 && src[i] != NULLCH; i++) {
    dst[i] = src[i];
  }
  dst[i] = NULLCH;
}

local uint32 *k2023202316_pd_from_phys(uint32 phys) {
  if (phys < K2023202316_KERNEL_IDENT_LIMIT) {
    return (uint32 *)phys;
  }
  return k2023202316_kmap(phys, 2);
}

local uint32 *k2023202316_pt_from_phys(uint32 phys) {
  if (phys < K2023202316_KERNEL_IDENT_LIMIT) {
    return (uint32 *)phys;
  }
  return k2023202316_kmap(phys, 3);
}

local uint32 k2023202316_ensure_pt(uint32 pd_phys, uint32 vaddr,
                                   uint32 flags) {
  uint32 *pd;
  uint32 pt_phys;
  uint32 pdi;
  pid32 pid;

  pd = k2023202316_pd_from_phys(pd_phys);
  pdi = vaddr >> 22;
  if (pd[pdi] & K2023202316_PTE_P) {
    return pd[pdi] & K2023202316_PAGE_MASK;
  }
  pid = k2023202316_pid_for_pd(pd_phys);
  pt_phys = k2023202316_alloc_frame("user-pt", pid, vaddr & 0xffc00000);
  if (pt_phys == 0) {
    return 0;
  }
  k2023202316_zero_frame(pt_phys);
  pd[pdi] = pt_phys | K2023202316_PTE_P | K2023202316_PTE_W |
            (flags & K2023202316_PTE_U);
  return pt_phys;
}

local uint32 k2023202316_lookup_page_raw(uint32 pd_phys, uint32 vaddr) {
  uint32 *pd;
  uint32 *pt;
  uint32 pde;

  pd = k2023202316_pd_from_phys(pd_phys);
  pde = pd[vaddr >> 22];
  if (!(pde & K2023202316_PTE_P)) {
    return 0;
  }
  pt = k2023202316_pt_from_phys(pde & K2023202316_PAGE_MASK);
  return pt[(vaddr >> 12) & 0x3ff];
}

local syscall k2023202316_unmap_page(pid32 pid, uint32 vaddr, bool8 free_frame,
                                     char *purpose) {
  struct procent *prptr;
  uint32 *pd;
  uint32 *pt;
  uint32 pde;
  uint32 pte;

  prptr = &proctab[pid];
  pd = k2023202316_pd_from_phys(prptr->pr2023202316_pdbr);
  pde = pd[vaddr >> 22];
  if (!(pde & K2023202316_PTE_P)) {
    return SYSERR;
  }
  pt = k2023202316_pt_from_phys(pde & K2023202316_PAGE_MASK);
  pte = pt[(vaddr >> 12) & 0x3ff];
  if (!(pte & K2023202316_PTE_P)) {
    return SYSERR;
  }
  if (free_frame) {
    k2023202316_free_frame(pte & K2023202316_PAGE_MASK, purpose, pid, vaddr);
  }
  pt[(vaddr >> 12) & 0x3ff] = 0;
  k2023202316_invlpg((void *)vaddr);
  return OK;
}

local syscall k2023202316_copy_bytes(pid32 pid, uint32 dst, uint32 src,
                                     void *buf, uint32 nbytes, bool8 to_user) {
  uint32 done;
  uint32 uaddr;
  uint32 pageoff;
  uint32 chunk;
  uint32 pte;
  uint8 *kptr;
  uint8 *mapped;

  for (done = 0; done < nbytes; done += chunk) {
    uaddr = to_user ? dst + done : src + done;
    pageoff = uaddr & K2023202316_PAGE_OFFSET;
    chunk = K2023202316_PAGE_SIZE - pageoff;
    if (chunk > nbytes - done) {
      chunk = nbytes - done;
    }
    pte = k2023202316_lookup_page(proctab[pid].pr2023202316_pdbr, uaddr);
    if (!(pte & K2023202316_PTE_P)) {
      return SYSERR;
    }
    mapped = (uint8 *)k2023202316_kmap(pte & K2023202316_PAGE_MASK, 0);
    kptr = (uint8 *)buf;
    if (to_user) {
      memcpy(mapped + pageoff, kptr + done, chunk);
    } else {
      memcpy(kptr + done, mapped + pageoff, chunk);
    }
    k2023202316_kunmap(0);
  }
  return OK;
}

local pid32 k2023202316_pid_for_pd(uint32 pd_phys) {
  pid32 pid;

  for (pid = 0; pid < NPROC; pid++) {
    if (proctab[pid].pr2023202316_pdbr == pd_phys) {
      return pid;
    }
  }
  return currpid;
}

local void k2023202316_set_name(struct procent *prptr, char *name) {
  int32 i;

  if (name == NULL) {
    name = "userproc";
  }
  prptr->prname[PNMLEN - 1] = NULLCH;
  for (i = 0; i < PNMLEN - 1 && name[i] != NULLCH; i++) {
    prptr->prname[i] = name[i];
  }
  if (i < PNMLEN) {
    prptr->prname[i] = NULLCH;
  }
}

local void k2023202316_copy_desc(pid32 src, pid32 dst) {
  int32 i;

  if ((src >= 0) && (src < NPROC)) {
    for (i = 0; i < NDESC; i++) {
      proctab[dst].prdesc[i] = proctab[src].prdesc[i];
    }
  } else {
    proctab[dst].prdesc[0] = CONSOLE;
    proctab[dst].prdesc[1] = CONSOLE;
    proctab[dst].prdesc[2] = CONSOLE;
    for (i = 3; i < NDESC; i++) {
      proctab[dst].prdesc[i] = -1;
    }
  }
}

local char *k2023202316_addr_to_purpose(pid32 pid, uint32 vaddr) {
  (void)pid;
  if (vaddr >= K2023202316_USTACK_TOP - K2023202316_USTACK_MAX &&
      vaddr < K2023202316_USTACK_TOP) {
    return "user-stack";
  }
  if (vaddr >= K2023202316_UHEAP_BASE && vaddr < K2023202316_UHEAP_LIMIT) {
    return "user-heap";
  }
  return "user-page";
}
