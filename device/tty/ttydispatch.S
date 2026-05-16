/* ttydispatch.S - ttydispatch */

#include <icu.s>
		.text
		.globl	ttydispatch
		.globl	ttyhandler

/*------------------------------------------------------------------------
 *  ttydispatch  -  Dispatch an interrupt to the tty handler function
 *------------------------------------------------------------------------
 */
ttydispatch:
		pushal			/* Save general-purpose regs.	*/
		pushfl			/* Save the flags register	*/
		cli			/* Prevent further interrupts	*/
		movw	$0x10,%ax	/* Lab3 2023202316 */
		movw	%ax,%ds		/* Lab3 2023202316 */
		movw	%ax,%es		/* Lab3 2023202316 */
		movw	%ax,%fs		/* Lab3 2023202316 */
		movw	%ax,%gs		/* Lab3 2023202316 */
		movb	$EOI,%al	/* Clear the interrupt		*/
		outb	%al,$OCW1_2
		//pushl	$0x0
		call	ttyhandler	/* Call the handler		*/
		//addl	$4, %esp
		sti
		popfl			/* Restore the flags register	*/
		movl	36(%esp),%eax	/* Lab3 2023202316 */
		andl	$3,%eax		/* Lab3 2023202316 */
		cmpl	$3,%eax		/* Lab3 2023202316 */
		jne	ttydispatch_kernel_return	/* Lab3 2023202316 */
		movw	$0x2B,%ax	/* Lab3 2023202316 */
		movw	%ax,%ds		/* Lab3 2023202316 */
		movw	%ax,%es		/* Lab3 2023202316 */
		movw	%ax,%fs		/* Lab3 2023202316 */
		movw	%ax,%gs		/* Lab3 2023202316 */
ttydispatch_kernel_return:		/* Lab3 2023202316 */
		popal			/* Restore general-purpose regs.*/
		iret			/* Return from interrupt	*/
