/* Lab5_dispatch.S - keyboard interrupt dispatch */

#include <icu.s>
		.text
		.globl	k2023202316_kbddisp
		.globl	k2023202316_kbdhandler

/*------------------------------------------------------------------------
 * k2023202316_kbddisp - Dispatch an IRQ1 interrupt to the keyboard handler
 *------------------------------------------------------------------------
 */
k2023202316_kbddisp:
		pushal
		pushfl
		cli
		movw	$0x10,%ax
		movw	%ax,%ds
		movw	%ax,%es
		movw	%ax,%fs
		movw	%ax,%gs
		movb	$EOI,%al
		outb	%al,$OCW1_2
		call	k2023202316_kbdhandler
		sti
		popfl
		movl	36(%esp),%eax
		andl	$3,%eax
		cmpl	$3,%eax
		jne	k2023202316_kbddisp_kernel_return
		movw	$0x2B,%ax
		movw	%ax,%ds
		movw	%ax,%es
		movw	%ax,%fs
		movw	%ax,%gs
k2023202316_kbddisp_kernel_return:
		popal
		iret
