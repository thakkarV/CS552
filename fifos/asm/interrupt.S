#define ASM_FILE 1
	
	.extern isr_timer
	.globl isr_32
isr_32:
	call isr_timer
	iret
