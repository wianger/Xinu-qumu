/* xsh_lab2.c - xsh_lab2 */

#include <xinu.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define K2023202316_NAME "wangyihang"
#define K2023202316_ID "2023202316"
#define K2023202316_DELAY_STACK 4096
#define K2023202316_FORK_HELPER_STACK 4096
#define K2023202316_EXEC_PRIO_BONUS 5

struct k2023202316_fork_state {
  volatile int32 role;
  volatile pid32 childpid;
  sid32 done;
};

local void k2023202316_copy_stdio(pid32);
local void k2023202316_set_name(struct procent *, char *);
local void k2023202316_wait_for(pid32);
local void k2023202316_fix_ebp_chain(uint32, uint32, uint32, uint32);
local status k2023202316_run_all(void);
local status k2023202316_run_delay_demo(void);
local status k2023202316_run_fork_demo(void);
local status k2023202316_run_exec_demo(void);
local void k2023202316_invoke(void *, uint32, int32, int32, int32, int32,
                              int32);
local char *k2023202316_build_stack(char *, uint32, void *, uint32, int32[]);
local process u2023202316_delay_worker(int32, void *, uint32, int32, int32,
                                       int32, int32, int32);
local process u2023202316_fork_helper(pid32, uint32);
local process u2023202316_fork_placeholder(void);
local process k2023202316_exec_bootstrap(char *, uint32, void *, uint32, int32,
                                         int32, int32, int32, int32);
local process u2023202316_fork_case(void);
local process u2023202316_exec_case(void);
local process u2023202316_exec_target(int32, int32);
local void u2023202316_delay_test(int32, int32, int32);
local void u2023202316_delay_test_one(int32);

/*------------------------------------------------------------------------
 * xsh_lab2 - run experiment 2 demonstrations
 *------------------------------------------------------------------------
 */
shellcmd xsh_lab2(int nargs, char *args[]) {
  status retval;

  retval = OK;

  if (nargs == 2 && strncmp(args[1], "--help", 7) == 0) {
    printf("Use:\n");
    printf("\t%s [delay|fork|exec|all]\n", args[0]);
    printf("Description:\n");
    printf("\tRun experiment 2 demonstrations for delay_run and fork/exec\n");
    printf("Options:\n");
    printf("\tdelay\trun only the asynchronous delay_run demo\n");
    printf("\tfork\trun only the fork demo\n");
    printf("\texec\trun only the fork+exec demo\n");
    printf("\tall\trun all demos (default)\n");
    printf("\t--help\tdisplay this help and exit\n");
    return SHELL_OK;
  }

  if (nargs > 2) {
    fprintf(stderr, "%s: too many arguments\n", args[0]);
    fprintf(stderr, "Try '%s --help' for more information\n", args[0]);
    return SHELL_ERROR;
  }

  if (nargs == 1 || strncmp(args[1], "all", 4) == 0) {
    retval = k2023202316_run_all();
  } else if (strncmp(args[1], "delay", 6) == 0) {
    retval = k2023202316_run_delay_demo();
  } else if (strncmp(args[1], "fork", 5) == 0) {
    retval = k2023202316_run_fork_demo();
  } else if (strncmp(args[1], "exec", 5) == 0) {
    retval = k2023202316_run_exec_demo();
  } else {
    fprintf(stderr, "%s: unknown mode '%s'\n", args[0], args[1]);
    fprintf(stderr, "Try '%s --help' for more information\n", args[0]);
    return SHELL_ERROR;
  }

  printf("lab2 done: %s %s\n", K2023202316_NAME, K2023202316_ID);
  return (retval == OK) ? SHELL_OK : SHELL_ERROR;
}

/*------------------------------------------------------------------------
 * k2023202316_delay_runv - asynchronously invoke a function later
 *------------------------------------------------------------------------
 */
syscall k2023202316_delay_runv(int32 seconds, void *func, uint32 nargs, ...) {
  va_list ap;
  int32 args[5];
  pid32 pid;
  uint32 i;

  if ((seconds < 0) || (func == NULL) || (nargs > 5)) {
    return SYSERR;
  }

  for (i = 0; i < 5; i++) {
    args[i] = 0;
  }

  va_start(ap, nargs);
  for (i = 0; i < nargs; i++) {
    args[i] = va_arg(ap, int32);
  }
  va_end(ap);

  pid = create((void *)u2023202316_delay_worker, K2023202316_DELAY_STACK,
               proctab[currpid].prprio, "lab2-delay", 8, seconds, func, nargs,
               args[0], args[1], args[2], args[3], args[4]);
  if (pid == SYSERR) {
    return SYSERR;
  }

  k2023202316_copy_stdio(pid);
  resume(pid);
  return OK;
}

/*------------------------------------------------------------------------
 * fork - clone the current execution context onto a new stack
 *
 * This implementation stays within Xinu's existing scheduler/process model:
 * a short-lived helper runs after the caller has been context-switched once,
 * copies the saved stack segment, and repairs the saved ebp chain so the child
 * can continue from the same call path. It intentionally does not scan stack
 * contents and rewrite pointer-like integers.
 *------------------------------------------------------------------------
 */
pid32 fork(void) {
  struct k2023202316_fork_state state;
  pid32 helper;
  pri16 helper_prio;

  state.role = 1;
  state.childpid = SYSERR;
  state.done = semcreate(0);
  if (state.done == SYSERR) {
    return SYSERR;
  }

  helper_prio = proctab[currpid].prprio + 1;
  helper = create((void *)u2023202316_fork_helper, K2023202316_FORK_HELPER_STACK,
                  helper_prio, "lab2-forkh", 2, currpid, (uint32)&state);
  if (helper == SYSERR) {
    semdelete(state.done);
    return SYSERR;
  }

  k2023202316_copy_stdio(helper);

  /* Keep the helper's exit notification off the caller's message slot. */
  proctab[helper].prparent = helper;
  resume(helper);

  if (state.role == 0) {
    return 0;
  }

  wait(state.done);
  semdelete(state.done);
  return state.childpid;
}

/*------------------------------------------------------------------------
 * exec - replace the current process context
 *------------------------------------------------------------------------
 */
void exec(void *funcaddr, pri16 priority, char *name, uint32 nargs, ...) {
  va_list ap;
  int32 args[9];
  struct procent *prptr;
  char *oldbase;
  uint32 oldlen;
  char *newbase;
  char *newsp;
  uint32 i;

  if ((funcaddr == NULL) || (priority < 1) || (nargs > 5)) {
    printf("exec: invalid arguments\n");
    return;
  }

  prptr = &proctab[currpid];
  oldbase = prptr->prstkbase;
  oldlen = prptr->prstklen;
  newbase = getstk(oldlen);
  if ((int32)newbase == SYSERR) {
    printf("exec: cannot allocate stack\n");
    return;
  }

  for (i = 0; i < 9; i++) {
    args[i] = 0;
  }
  args[0] = (int32)oldbase;
  args[1] = (int32)oldlen;
  args[2] = (int32)funcaddr;
  args[3] = (int32)nargs;

  va_start(ap, nargs);
  for (i = 0; i < nargs; i++) {
    args[4 + i] = va_arg(ap, int32);
  }
  va_end(ap);

  newsp = k2023202316_build_stack(newbase, oldlen,
                                  (void *)k2023202316_exec_bootstrap, 9, args);
  if ((int32)newsp == SYSERR) {
    freestk(newbase, oldlen);
    printf("exec: cannot build stack\n");
    return;
  }

  disable();
  prptr->prprio = priority;
  k2023202316_set_name(prptr, name);
  prptr->prstkbase = newbase;
  prptr->prstklen = oldlen;
  prptr->prstkptr = newsp;
  preempt = QUANTUM;

  asm volatile("movl %0, %%esp\n\t"
               "popal\n\t"
               "movl 4(%%esp), %%ebp\n\t"
               "popfl\n\t"
               "addl $4, %%esp\n\t"
               "ret\n\t"
               :
               : "r"(newsp)
               : "memory");

  while (TRUE) {
  }
}

/*------------------------------------------------------------------------
 * k2023202316_run_all - run all demos in sequence
 *------------------------------------------------------------------------
 */
local status k2023202316_run_all(void) {
  if (k2023202316_run_delay_demo() == SYSERR) {
    return SYSERR;
  }
  if (k2023202316_run_fork_demo() == SYSERR) {
    return SYSERR;
  }
  if (k2023202316_run_exec_demo() == SYSERR) {
    return SYSERR;
  }
  return OK;
}

/*------------------------------------------------------------------------
 * k2023202316_run_delay_demo - test asynchronous delay_run
 *------------------------------------------------------------------------
 */
local status k2023202316_run_delay_demo(void) {
  printf("\n[lab2] delay_run demo\n");
  printf("xsh_lab2(1): %d.%d\n", clktime, count1000);
  if (delay_run(2, u2023202316_delay_test, 11, 22, 33) == SYSERR) {
    printf("delay_run call 1 failed\n");
    return SYSERR;
  }
  if (delay_run(4, u2023202316_delay_test_one, 44) == SYSERR) {
    printf("delay_run call 2 failed\n");
    return SYSERR;
  }
  if (delay_run(6, u2023202316_delay_test, 55, 66, 77) == SYSERR) {
    printf("delay_run call 3 failed\n");
    return SYSERR;
  }
  printf("xsh_lab2(2): %d.%d\n", clktime, count1000);
  sleep(7);
  return OK;
}

/*------------------------------------------------------------------------
 * k2023202316_run_fork_demo - test fork without exec
 *------------------------------------------------------------------------
 */
local status k2023202316_run_fork_demo(void) {
  pid32 pid;

  printf("\n[lab2] fork demo\n");
  recvclr();
  pid = create((void *)u2023202316_fork_case, K2023202316_DELAY_STACK,
               proctab[currpid].prprio, "lab2-fork", 0);
  if (pid == SYSERR) {
    printf("cannot start fork case\n");
    return SYSERR;
  }

  k2023202316_copy_stdio(pid);
  resume(pid);
  k2023202316_wait_for(pid);
  return OK;
}

/*------------------------------------------------------------------------
 * k2023202316_run_exec_demo - test fork followed by exec
 *------------------------------------------------------------------------
 */
local status k2023202316_run_exec_demo(void) {
  pid32 pid;

  printf("\n[lab2] exec demo\n");
  recvclr();
  pid = create((void *)u2023202316_exec_case, K2023202316_DELAY_STACK,
               proctab[currpid].prprio, "lab2-exec", 0);
  if (pid == SYSERR) {
    printf("cannot start exec case\n");
    return SYSERR;
  }

  k2023202316_copy_stdio(pid);
  resume(pid);
  k2023202316_wait_for(pid);
  return OK;
}

/*------------------------------------------------------------------------
 * k2023202316_copy_stdio - inherit current process standard descriptors
 *------------------------------------------------------------------------
 */
local void k2023202316_copy_stdio(pid32 pid) {
  int32 i;

  if (isbadpid(pid)) {
    return;
  }

  for (i = 0; i < NDESC; i++) {
    proctab[pid].prdesc[i] = proctab[currpid].prdesc[i];
  }
}

/*------------------------------------------------------------------------
 * k2023202316_set_name - safely store a process name
 *------------------------------------------------------------------------
 */
local void k2023202316_set_name(struct procent *prptr, char *name) {
  int32 i;

  if (name == NULL) {
    name = "lab2-exec";
  }

  prptr->prname[PNMLEN - 1] = NULLCH;
  for (i = 0; i < PNMLEN - 1 && name[i] != NULLCH; i++) {
    prptr->prname[i] = name[i];
  }
  if (i < PNMLEN) {
    prptr->prname[i] = NULLCH;
  }
}

/*------------------------------------------------------------------------
 * k2023202316_wait_for - wait until a specific child finishes
 *------------------------------------------------------------------------
 */
local void k2023202316_wait_for(pid32 target) {
  pid32 msg;

  if ((target < 0) || (target >= NPROC)) {
    return;
  }
  while (TRUE) {
    msg = receive();
    if (msg == target) {
      return;
    }
  }
}

/*------------------------------------------------------------------------
 * k2023202316_fix_ebp_chain - retarget saved frame pointers after copy
 *------------------------------------------------------------------------
 */
local void k2023202316_fix_ebp_chain(uint32 parent_low, uint32 parent_high,
                                     uint32 child_low, uint32 used) {
  uint32 delta;
  uint32 child_high;
  uint32 *saved_ebp;
  uint32 frame;
  uint32 prev;

  if (used <= 40) {
    return;
  }

  delta = child_low - parent_low;
  child_high = child_low + used;
  saved_ebp = (uint32 *)(child_low + 36);

  if ((*saved_ebp < parent_low) || (*saved_ebp >= parent_high)) {
    return;
  }

  *saved_ebp += delta;
  frame = *saved_ebp;

  while ((frame >= child_low) && ((frame + sizeof(uint32)) <= child_high)) {
    prev = *(uint32 *)frame;
    if ((prev < parent_low) || (prev >= parent_high)) {
      break;
    }
    *(uint32 *)frame = prev + delta;
    frame = prev + delta;
  }
}

/*------------------------------------------------------------------------
 * k2023202316_invoke - dispatch a function pointer with up to five ints
 *------------------------------------------------------------------------
 */
local void k2023202316_invoke(void *func, uint32 nargs, int32 a1, int32 a2,
                              int32 a3, int32 a4, int32 a5) {
  switch (nargs) {
  case 0:
    ((void (*)(void))func)();
    break;
  case 1:
    ((void (*)(int32))func)(a1);
    break;
  case 2:
    ((void (*)(int32, int32))func)(a1, a2);
    break;
  case 3:
    ((void (*)(int32, int32, int32))func)(a1, a2, a3);
    break;
  case 4:
    ((void (*)(int32, int32, int32, int32))func)(a1, a2, a3, a4);
    break;
  case 5:
    ((void (*)(int32, int32, int32, int32, int32))func)(a1, a2, a3, a4, a5);
    break;
  default:
    printf("invoke: unsupported nargs=%d\n", nargs);
    break;
  }
}

/*------------------------------------------------------------------------
 * k2023202316_build_stack - create a Xinu-style initial stack frame
 *------------------------------------------------------------------------
 */
local char *k2023202316_build_stack(char *stkbase, uint32 ssize, void *funcaddr,
                                    uint32 nargs, int32 args[]) {
  uint32 savsp;
  uint32 *pushsp;
  uint32 *saddr;
  uint32 i;

  if ((stkbase == NULL) || (funcaddr == NULL) || (nargs > 10)) {
    return (char *)SYSERR;
  }
  (void)ssize;

  saddr = (uint32 *)stkbase;
  *saddr = STACKMAGIC;
  savsp = (uint32)saddr;

  for (i = nargs; i > 0; i--) {
    *--saddr = (uint32)args[i - 1];
  }
  *--saddr = (uint32)INITRET;
  *--saddr = (uint32)funcaddr;
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

  return (char *)saddr;
}

/*------------------------------------------------------------------------
 * u2023202316_delay_worker - sleep and then invoke the target callback
 *------------------------------------------------------------------------
 */
local process u2023202316_delay_worker(int32 seconds, void *func, uint32 nargs,
                                       int32 a1, int32 a2, int32 a3, int32 a4,
                                       int32 a5) {
  sleep(seconds);
  k2023202316_invoke(func, nargs, a1, a2, a3, a4, a5);
  return OK;
}

/*------------------------------------------------------------------------
 * u2023202316_fork_helper - clone the parent stack into a new process
 *------------------------------------------------------------------------
 */
local process u2023202316_fork_helper(pid32 parentpid, uint32 state_addr) {
  intmask mask;
  struct procent *parent;
  struct procent *child;
  struct k2023202316_fork_state *parent_state;
  struct k2023202316_fork_state *child_state;
  pid32 childpid;
  uint32 parent_low;
  uint32 parent_high;
  uint32 used;
  uint32 child_low;
  uint32 offset;
  uint32 i;

  mask = disable();
  parent = &proctab[parentpid];
  parent_state = (struct k2023202316_fork_state *)state_addr;

  childpid = create((void *)u2023202316_fork_placeholder, parent->prstklen,
                    parent->prprio, parent->prname, 0);
  if (childpid == SYSERR) {
    parent_state->childpid = SYSERR;
    signal(parent_state->done);
    restore(mask);
    return SYSERR;
  }

  child = &proctab[childpid];
  parent_low = (uint32)parent->prstkptr;
  parent_high = (uint32)parent->prstkbase + sizeof(uint32);
  used = parent_high - parent_low;
  child_low = ((uint32)child->prstkbase + sizeof(uint32)) - used;

  memcpy((void *)child_low, (void *)parent_low, used);
  child->prstkptr = (char *)child_low;
  child->prparent = parentpid;
  child->prprio = parent->prprio;
  child->prhasmsg = FALSE;
  child->prmsg = 0;
  child->prsem = -1;
  for (i = 0; i < NDESC; i++) {
    child->prdesc[i] = parent->prdesc[i];
  }

  k2023202316_fix_ebp_chain(parent_low, parent_high, child_low, used);

  offset = state_addr - parent_low;
  child_state = (struct k2023202316_fork_state *)(child_low + offset);
  child_state->role = 0;
  child_state->childpid = 0;

  parent_state->childpid = childpid;
  signal(parent_state->done);
  ready(childpid);
  restore(mask);
  return OK;
}

/*------------------------------------------------------------------------
 * u2023202316_fork_placeholder - should never run if cloning succeeds
 *------------------------------------------------------------------------
 */
local process u2023202316_fork_placeholder(void) { return OK; }

/*------------------------------------------------------------------------
 * k2023202316_exec_bootstrap - free the old stack and run new code
 *------------------------------------------------------------------------
 */
local process k2023202316_exec_bootstrap(char *oldstkbase, uint32 oldstklen,
                                         void *func, uint32 nargs, int32 a1,
                                         int32 a2, int32 a3, int32 a4,
                                         int32 a5) {
  freestk(oldstkbase, oldstklen);
  k2023202316_invoke(func, nargs, a1, a2, a3, a4, a5);
  return OK;
}

/*------------------------------------------------------------------------
 * u2023202316_fork_case - verify fork return values and names
 *------------------------------------------------------------------------
 */
local process u2023202316_fork_case(void) {
  pid32 pid;

  pid = fork();
  if (pid == SYSERR) {
    printf("fork failed\n");
    return SYSERR;
  }

  printf("curr proc: %d(%d), %s\n", pid, currpid, proctab[currpid].prname);
  if (pid > 0) {
    k2023202316_wait_for(pid);
  }
  return OK;
}

/*------------------------------------------------------------------------
 * u2023202316_exec_case - verify fork followed by exec
 *------------------------------------------------------------------------
 */
local process u2023202316_exec_case(void) {
  pid32 pid;

  pid = fork();
  if (pid == 0) {
    exec((void *)u2023202316_exec_target,
         proctab[currpid].prprio + K2023202316_EXEC_PRIO_BONUS,
         "exec-child", 2, 2023, currpid);
    printf("exec returned unexpectedly in pid %d\n", currpid);
    return SYSERR;
  }
  if (pid < 0) {
    printf("fork failed in exec case\n");
    return SYSERR;
  }

  printf("parent proc: %d(%d), %s\n", pid, currpid, proctab[currpid].prname);
  k2023202316_wait_for(pid);
  printf("currpid = %d\n", currpid);
  return OK;
}

/*------------------------------------------------------------------------
 * u2023202316_exec_target - target used by exec() testing
 *------------------------------------------------------------------------
 */
local process u2023202316_exec_target(int32 a, int32 b) {
  printf("exec target: pid=%d, name=%s, args=(%d,%d)\n", currpid,
         proctab[currpid].prname, a, b);
  return OK;
}

/*------------------------------------------------------------------------
 * u2023202316_delay_test - sample callback for delay_run
 *------------------------------------------------------------------------
 */
local void u2023202316_delay_test(int32 a, int32 b, int32 c) {
  printf("delay_test: %d.%d\n", clktime, count1000);
  printf("delay_test args: %d %d %d\n", a, b, c);
}

/*------------------------------------------------------------------------
 * u2023202316_delay_test_one - alternate callback with one argument
 *------------------------------------------------------------------------
 */
local void u2023202316_delay_test_one(int32 a) {
  printf("delay_test: %d.%d\n", clktime, count1000);
  printf("delay_test arg: %d\n", a);
}
