#define ASM_FILE 1
	
	.extern do_timer, do_reserved, do_unimpl, do_double_fault
	.extern schedule, jiffies, sched_finalize_thread, __current_task
	.globl isr_timer, isr_reserved, isr_unimpl, isr_double_fault, isr_kbd


isr_timer:

.save_ctx:
	// pushf
	// push %eax
	// push %ebx
	// push %ecx
	// push %edx
	// push %esi
	// push %edi
	// push %ebp
	// push %ss
	// push %ds
	// push %fs
	// push %gs
	// push %es

	/* send eoi */
	movb $0x20, %al
	outb %al, $0x20
	
	call do_timer

	// pop %es
	// pop %gs
	// pop %fs
	// pop %ds
	// pop %ss
	// popl %ebp
	// popl %edi
	// popl %esi
	// popl %edx
	// popl %ecx
	// popl %ebx
	// popl %eax

	iret

	// // check if it is a new therad save
	// pushl %eax
	// pushl %ebx
	// pushl %ebx

	// movl $0x01, %eax
	// movl __current_task, %ecx
	// movl 24(%ecx), %ebx
	// cmp %eax, %ebx

	// popl %ecx
	// popl %ebx
	// popl %eax
	// jne .start_new

// 	movl $0x01, __current_task

// .swap_esp:
// 	movl __current_task, %esp		/* moves esp of current task into current esp */

// start_new:
// 	call sched
	

// .resume:
// 	/* send eoi */
// 	movb $0x20, %al
// 	outb %al, $0x20
// 	iret

// no_schedule_ret:
	
// 	/* send eoi */
// 	movb $0x20, %al
// 	outb %al, $0x20

// 	popl %eax
// 	popl %ebx
// 	popl %ecx
// 	popl %edx
// 	pop %fs
// 	pop %es
// 	pop %ds
// 	popf

// 	iret


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
