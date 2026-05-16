/* clkdisp.s - clkdisp (x86) */

/*------------------------------------------------------------------------
 * clkdisp  -  Interrupt dispatcher for clock interrupts (x86 version)
 *------------------------------------------------------------------------
 */
#include <icu.s>
		.text
		.globl	clkdisp		# Clock interrupt dispatcher
clkdisp:
		pushal			# Save registers
		cli			# Disable further interrupts
		movw	$0x10,%ax	# Lab3 2023202316
		movw	%ax,%ds		# Lab3 2023202316
		movw	%ax,%es		# Lab3 2023202316
		movw	%ax,%fs		# Lab3 2023202316
		movw	%ax,%gs		# Lab3 2023202316
		movb	$EOI,%al	# Reset interrupt
		outb	%al,$OCW1_2

		call	clkhandler	# Call high level handler

		sti			# Restore interrupt status
		movl	36(%esp),%eax	# Lab3 2023202316
		andl	$3,%eax		# Lab3 2023202316
		cmpl	$3,%eax		# Lab3 2023202316
		jne	clkdisp_kernel_return	# Lab3 2023202316
		movw	$0x2B,%ax	# Lab3 2023202316
		movw	%ax,%ds		# Lab3 2023202316
		movw	%ax,%es		# Lab3 2023202316
		movw	%ax,%fs		# Lab3 2023202316
		movw	%ax,%gs		# Lab3 2023202316
clkdisp_kernel_return:			# Lab3 2023202316
		popal			# Restore registers
		iret			# Return from interrupt
