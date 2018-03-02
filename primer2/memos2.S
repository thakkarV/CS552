/* constants for multiboot header */
.set ALIGN,     1<<0
.set MEMINFO,   1<<1
.set FLAGS,     ALIGN | MEMINFO
.set MAGIC,     0x1BADB002
.set CHECKSUM,  -(MAGIC+FLAGS)

/* multiboot header with params */
.section .multiboot
    .align 4
    .long MAGIC
    .long FLAGS
    .long CHECKSUM

/* setup 16KB stack */
/* x86 stack MUST be 16-byte aligned, acc to System V ABI standard and de-facto extensions */
.section .bss
    .align 4
stack_bottom:
    .skip 16384
stack_top:

.section .text
.global _start

_start:

    mov $stack_top, %esp    /* make stack pointer point to top of stack */
    call kernel_main        /* our kernel is ready to be executed */

1:	hlt
	jmp 1b
