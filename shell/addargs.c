/* addargs.c - addargs */

#include <xinu.h>
#include "shprototypes.h"

/*------------------------------------------------------------------------
 *  addargs  -  Add local copy of argv-style arguments to the stack of
 *		  a command process that has been created by the shell
 *------------------------------------------------------------------------
 */
status addargs(pid32 pid,    /* ID of process to use		*/
               int32 ntok,   /* Count of arguments		*/
               int32 tok[],  /* Index of tokens in tokbuf	*/
               int32 tlen,   /* Length of data in tokbuf	*/
               char *tokbuf, /* Array of null-term. tokens	*/
               void *dummy   /* Dummy argument that was	*/
                             /*   used at creation and must	*/
                             /*   be replaced by a pointer	*/
                             /*   to an argument vector	*/
) {
  intmask mask;          /* Saved interrupt mask		*/
  struct procent *prptr; /* Ptr to process' table entry	*/
  uint32 aloc;           /* Argument location in process	*/
                         /*   stack as an integer	*/
  uint32 *argloc;        /* Location in process's stack	*/
                         /*   to place args vector	*/
  char *argstr;          /* Location in process's stack	*/
                         /*   to place arg strings	*/
  uint32 *search;        /* pointer that searches for	*/
                         /*   dummy argument on stack	*/
  uint32 *aptr;          /* Walks through args array	*/
  int32 i;               /* Index into tok array		*/
  uint32 uargv[SHELL_MAXTOK + 1]; // Lab4 2023202316

  mask = disable();

  /* Check argument count and data length */

  if ((ntok <= 0) || (tlen < 0)) {
    restore(mask);
    return SYSERR;
  }

  prptr = &proctab[pid];

  /*Lab3 2023202316: Begin*/
  if (prptr->pr2023202316_isuser) {
    aloc = (uint32)(prptr->pr2023202316_ustackbase + sizeof(uint32));
    argloc = (uint32 *)((aloc + 3) & ~0x3);
    argstr = (char *)(argloc + (ntok + 1));

    for (i = 0; i < ntok; i++) {
      uargv[i] = (uint32)(argstr + tok[i]);
    }
    uargv[ntok] = (uint32)NULL;
    if (k2023202316_copy_to_user(pid, (uint32)argloc, uargv,
                                 (ntok + 1) * sizeof(uint32)) == SYSERR ||
        k2023202316_copy_to_user(pid, (uint32)argstr, tokbuf, tlen) ==
            SYSERR) {
      restore(mask);
      return SYSERR;
    }

    for (search = (uint32 *)prptr->pr2023202316_ustkptr;
         search < (uint32 *)prptr->pr2023202316_ustkbase; search++) {
      uint32 word;
      if (k2023202316_copy_from_user(pid, &word, (uint32)search,
                                     sizeof(uint32)) == SYSERR) {
        restore(mask);
        return SYSERR;
      }
      if (word == (uint32)dummy) {
        word = (uint32)argloc;
        if (k2023202316_copy_to_user(pid, (uint32)search, &word,
                                     sizeof(uint32)) == SYSERR) {
          restore(mask);
          return SYSERR;
        }
        restore(mask);
        return OK;
      }
    }

    restore(mask);
    return SYSERR;
  }
  /*Lab3 2023202316: End*/

  /* Compute lowest location in the process stack where the	*/
  /*	args array will be stored followed by the argument	*/
  /*	strings							*/

  aloc = (uint32)(prptr->prstkbase - prptr->prstklen + sizeof(uint32));
  argloc = (uint32 *)((aloc + 3) & ~0x3); /* round multiple of 4	*/

  /* Compute the first location beyond args array for the strings	*/

  argstr = (char *)(argloc + (ntok + 1)); /* +1 for a null ptr	*/

  /* Set each location in the args vector to be the address of	*/
  /*	string area plus the offset of this argument		*/

  for (aptr = argloc, i = 0; i < ntok; i++) {
    *aptr++ = (uint32)(argstr + tok[i]);
  }

  /* Add a null pointer to the args array */

  *aptr++ = (uint32)NULL;

  /* Copy the argument strings from tokbuf into process's	stack	*/
  /*	just beyond the args vector				*/

  memcpy(aptr, tokbuf, tlen);

  /* Find the second argument in process's stack */

  for (search = (uint32 *)prptr->prstkptr; search < (uint32 *)prptr->prstkbase;
       search++) {

    /* If found, replace with the address of the args vector*/

    if (*search == (uint32)dummy) {
      *search = (uint32)argloc;
      restore(mask);
      return OK;
    }
  }

  /* Argument value not found on the stack - report an error */

  restore(mask);
  return SYSERR;
}
