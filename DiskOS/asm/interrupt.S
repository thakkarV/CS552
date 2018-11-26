#define ASM_FILE 1
	
	.extern do_timer, do_unimpl
	.extern schedule, jiffies, sched_finalize_thread, __current_task

	.extern do_divide_error, do_debug, do_nmi, do_int3, do_overflow
	.extern do_bounds, do_invalid_op, do_device_unavailable, do_double_fault
	.extern do_coprocessor_segment_overrun, do_invalid_tss, do_segment_not_present
	.extern do_stack_segment_fault, do_general_protection_fault, do_page_fault
	.extern do_reserved, do_x87_fpu_except

	.globl isr_timer, isr_unimpl, isr_kbd
	.globl isr_debug, isr_nmi, isr_int3, isr_overflow, isr_bounds, isr_invalid_op
	.globl isr_divide_error, isr_debug, isr_nmi, isr_int3, isr_overflow, isr_bounds
	.globl isr_invalid_op, isr_device_unavailable, isr_double_fault
	.globl isr_coprocessor_segment_overrun, isr_invalid_tss, isr_segment_not_present
	.globl isr_stack_segment_fault, isr_general_protection_fault, isr_page_fault
	.globl isr_reserved, isr_x87_fpu_except

# system IRQ ISRs
isr_timer:
	# send eoi
	movb $0x20, %al
	outb %al, $0x20
	call do_timer
	iret

isr_kbd:
	movb $0x20, %al
	outb %al, $0x20
	iret


# faults, traps and exceptions in increasing order
isr_divide_error:
	pushl $do_divide_error
	jmp no_error_code

isr_debug:
	pushl $do_debug
	jmp no_error_code

isr_nmi:
	pushl $do_nmi
	jmp no_error_code

isr_int3:
	pushl $do_int3
	jmp no_error_code

isr_overflow:
	pushl $do_overflow
	jmp no_error_code

isr_bounds:
	pushl $do_bounds
	jmp no_error_code

isr_invalid_op:
	pushl $do_invalid_op
	jmp no_error_code

isr_device_unavailable:
	pushl $do_device_unavailable
	jmp no_error_code

isr_double_fault:
	pushl $do_double_fault
	jmp error_code

isr_coprocessor_segment_overrun:
	pushl $do_coprocessor_segment_overrun
	jmp no_error_code

isr_invalid_tss:
	pushl $do_invalid_tss
	jmp error_code

isr_segment_not_present:
	pushl $do_segment_not_present
	jmp error_code

isr_stack_segment_fault:
	pushl $do_stack_segment_fault
	jmp error_code

isr_general_protection_fault:
	pushl $do_general_protection_fault
	jmp error_code

isr_page_fault:
	pushl $do_page_fault
	jmp error_code

isr_reserved:
	pushl $do_reserved
	jmp no_error_code

isr_x87_fpu_except:
	pushl $do_x87_fpu_except
	jmp no_error_code

error_code:
	xchgl %eax, 4(%esp) # error code <-> %eax
	xchgl %ebx, (%esp) # &function <-> %ebx
	pushl %ecx
	pushl %edx
	pushl %edi
	pushl %esi
	pushl %ebp
	push %ds
	push %es
	push %fs
	pushl %eax # error code
	lea 44(%esp), %eax # offset
	pushl %eax
	movl $0x10, %eax
	mov %ax,%ds
	mov %ax,%es
	mov %ax,%fs

	call *%ebx

	addl $8,%esp
	pop %fs
	pop %es
	pop %ds
	popl %ebp
	popl %esi
	popl %edi
	popl %edx
	popl %ecx
	popl %ebx
	popl %eax
	iret

no_error_code:
	xchgl %eax, (%esp) # &function <-> %eax
	pushl %ebx
	pushl %ecx
	pushl %edx
	pushl %edi
	pushl %esi
	pushl %ebp
	push %ds
	push %es
	push %fs
	pushl $0 # "error code"
	lea 44(%esp), %edx
	pushl %edx
	movl $0x10, %edx
	mov %dx,%ds
	mov %dx,%es
	mov %dx,%fs

	call *%eax

	addl $8,%esp
	pop %fs
	pop %es
	pop %ds
	popl %ebp
	popl %esi
	popl %edi
	popl %edx
	popl %ecx
	popl %ebx
	popl %eax
	iret
