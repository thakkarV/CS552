#define ASM_FILE 1
	
	.extern do_timer, do_reserved, do_unimpl, do_double_fault
	.globl isr_timer, isr_reserved, isr_unimpl, isr_double_fault, isr_kbd


isr_timer:
	pushf
	push %ds
	push %es
	push %fs
	pushl %edx
	pushl %ecx
	pushl %ebx
	pushl %eax

	call do_timer

	# EOI to interrupt controller #1
	movb $0x20, %al
	outb %al, $0x20

	popl %eax
	popl %ebx
	popl %ecx
	popl %edx
	pop %fs
	pop %es
	pop %ds
	popf

	iret


isr_reserved:
	pushf
	push %ds
	push %es
	push %fs
	pushl %edx
	pushl %ecx
	pushl %ebx
	pushl %eax

	call do_reserved

	# EOI to interrupt controller #1
	movb $0x20, %al
	outb %al, $0x20

	popl %eax
	popl %ebx
	popl %ecx
	popl %edx
	pop %fs
	pop %es
	pop %ds
	popf

	iret


isr_kbd:
	movb $0x20, %al
	outb %al, $0x20
	iret

isr_unimpl:
	pushf
	push %ds
	push %es
	push %fs
	pushl %edx
	pushl %ecx
	pushl %ebx
	pushl %eax

	call do_unimpl

	popl %eax
	popl %ebx
	popl %ecx
	popl %edx
	pop %fs
	pop %es
	pop %ds
	popf

	iret


isr_double_fault:
	pushf
	push %ds
	push %es
	push %fs
	pushl %edx
	pushl %ecx
	pushl %ebx
	pushl %eax

	call do_double_fault

	popl %eax
	popl %ebx
	popl %ecx
	popl %edx
	pop %fs
	pop %es
	pop %ds
	popf

	iret
