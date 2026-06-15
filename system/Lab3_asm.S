/* Lab3_asm.S - experiment 3 privilege transition helpers */

#define K2023202316_KCODE_SEL 0x08
#define K2023202316_KDATA_SEL 0x10
#define K2023202316_KSTACK_SEL 0x18
#define K2023202316_UCODE_SEL 0x23
#define K2023202316_UDATA_SEL 0x2B
#define K2023202316_TSS_SEL 0x30

		.text
		.globl	k2023202316_ltr
		.globl	k2023202316_iret_to_user
		.globl	k2023202316_syscall_entry

k2023202316_ltr:
		movw	$K2023202316_TSS_SEL,%ax
		ltr	%ax
		ret

k2023202316_iret_to_user:
		movl	4(%esp),%eax
		movl	8(%esp),%edx
		movw	$K2023202316_UDATA_SEL,%cx
		movw	%cx,%ds
		movw	%cx,%es
		movw	%cx,%fs
		movw	%cx,%gs
		pushl	$K2023202316_UDATA_SEL
		pushl	%edx
		pushfl
		popl	%ecx
		orl	$0x00000200,%ecx
		pushl	%ecx
		pushl	$K2023202316_UCODE_SEL
		pushl	%eax
		iret

k2023202316_syscall_entry:
		pushal
		movl	%esp,%ebp
		movw	$K2023202316_KDATA_SEL,%ax
		movw	%ax,%ds
		movw	%ax,%es
		movw	%ax,%fs
		movw	%ax,%gs

		pushl	%ebp
		call	k2023202316_syscall_dispatch
		addl	$4,%esp
		movl	%eax,28(%ebp)

		movw	$K2023202316_UDATA_SEL,%ax
		movw	%ax,%ds
		movw	%ax,%es
		movw	%ax,%fs
		movw	%ax,%gs
		movl	%ebp,%esp
		popal
		iret
