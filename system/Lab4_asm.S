/* Lab4_asm.S - paging and trap helpers */

#define K2023202316_KDATA_SEL 0x10
#define K2023202316_UDATA_SEL 0x2B

		.text
		.globl	k2023202316_load_cr3
		.globl	k2023202316_read_cr0
		.globl	k2023202316_write_cr0
		.globl	k2023202316_read_cr2
		.globl	k2023202316_invlpg
		.globl	k2023202316_restore_trapframe
		.globl	k2023202316_page_fault_entry

k2023202316_load_cr3:
		movl	4(%esp),%eax
		movl	%eax,%cr3
		ret

k2023202316_read_cr0:
		movl	%cr0,%eax
		ret

k2023202316_write_cr0:
		movl	4(%esp),%eax
		movl	%eax,%cr0
		ret

k2023202316_read_cr2:
		movl	%cr2,%eax
		ret

k2023202316_invlpg:
		movl	4(%esp),%eax
		invlpg	(%eax)
		ret

k2023202316_restore_trapframe:
		movl	4(%esp),%esp
		movw	$K2023202316_UDATA_SEL,%ax
		movw	%ax,%ds
		movw	%ax,%es
		movw	%ax,%fs
		movw	%ax,%gs
		popal
		iret

k2023202316_page_fault_entry:
		pushal
		movl	%esp,%ebp
		movw	$K2023202316_KDATA_SEL,%ax
		movw	%ax,%ds
		movw	%ax,%es
		movw	%ax,%fs
		movw	%ax,%gs
		pushl	32(%ebp)
		pushl	%ebp
		call	k2023202316_page_fault_handler
		addl	$8,%esp
		cmpl	$0,%eax
		je	k2023202316_page_fault_halt
		movl	40(%ebp),%eax
		andl	$3,%eax
		cmpl	$3,%eax
		jne	k2023202316_page_fault_kernel_return
		movw	$K2023202316_UDATA_SEL,%ax
		movw	%ax,%ds
		movw	%ax,%es
		movw	%ax,%fs
		movw	%ax,%gs
k2023202316_page_fault_kernel_return:
		movl	%ebp,%esp
		popal
		addl	$4,%esp
		iret

k2023202316_page_fault_halt:
		jmp	k2023202316_page_fault_halt
