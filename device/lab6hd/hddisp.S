/* hddisp.S - k2023202316_hddisp */

#include <icu.s>
		.text
		.globl	k2023202316_hddisp
		.globl	k2023202316_hdhandler

/*------------------------------------------------------------------------
 * k2023202316_hddisp - Dispatch an IRQ14 disk interrupt
 *------------------------------------------------------------------------
 */
k2023202316_hddisp:
		pushal
		pushfl
		cli
		movw	$0x10,%ax
		movw	%ax,%ds
		movw	%ax,%es
		movw	%ax,%fs
		movw	%ax,%gs
		movb	$EOI,%al
		outb	%al,$OCW2_2
		movb	$EOI,%al
		outb	%al,$OCW1_2
		call	k2023202316_hdhandler
		sti
		popfl
		movl	36(%esp),%eax
		andl	$3,%eax
		cmpl	$3,%eax
		jne	k2023202316_hddisp_kernel_return
		movw	$0x2B,%ax
		movw	%ax,%ds
		movw	%ax,%es
		movw	%ax,%fs
		movw	%ax,%gs
k2023202316_hddisp_kernel_return:
		popal
		iret
