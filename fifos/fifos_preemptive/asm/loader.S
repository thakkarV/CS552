#define ASM_FILE 1
#include <multiboot.h>

/* C symbol format. HAVE_ASM_USCORE is defined by configure. */
#ifdef HAVE_ASM_USCORE
#define EXT_C(sym) _ ## sym
#else
#define EXT_C(sym) sym
#endif

/* The size of our stack (16KB). */
#define STACK_SIZE 0x4000

/* The flags for the Multiboot header. */
#ifdef __ELF__
#define AOUT_KLUDGE 0
#else
#define AOUT_KLUDGE MULTIBOOT_AOUT_KLUDGE
#endif
#define MULTIBOOT_HEADER_FLAGS MULTIBOOT_PAGE_ALIGN | MULTIBOOT_MEMORY_INFO | MULTIBOOT_VIDEO_MODE | AOUT_KLUDGE

.globl  start, _start, gdt


/*
 * MULTIBOOT HEADER
*/
.section .multiboot
	/* magic */
	.long   MULTIBOOT_HEADER_MAGIC
	/* flags */
	.long   MULTIBOOT_HEADER_FLAGS
	/* checksum */
	.long   -(MULTIBOOT_HEADER_MAGIC + MULTIBOOT_HEADER_FLAGS)
#ifndef __ELF__
	/* header_addr */
	.long   multiboot_header
	/* load_addr */
	.long   _start
	/* load_end_addr */
	.long   _edata
	/* bss_end_addr */
	.long   _end
	/* entry_addr */
	.long   multiboot_entry
#else /* ! __ELF__ */
	.long   0
	.long   0
	.long   0
	.long   0
	.long   0
#endif /* __ELF__ */
	.long   0
	.long   1024
	.long   768
	.long   32


/*
 * TEXT
*/
.section .text
/* Setup GDT */
gdt:
	/* first section discriptor is null */
	.long   0
	.long   0

	/* KERNEL CODE */
	.long   0x0000FFFF
	.long   0x00CF9A00

	/* KERNEL DATA */
	.long   0x0000FFFF
	.long   0x00CF9200

gdt_ptr:
	.short  0x17
	.long   gdt

start:
_start:
	jmp     multiboot_entry

	.align  4

multiboot_entry:
	/* Initialize the stack pointer. */
	movl    $(stack + STACK_SIZE), %esp

	/* Enable GDT */
	lgdt    gdt_ptr
	push    %ax
	ljmp    $0x08, $1f
1:
	movw    $0x10, %ax
	movw    %ax, %ss
	movw    %ax, %ds
	movw    %ax, %es
	movw    %ax, %fs
	movw    %ax, %gs
	pop     %ax

	/* Reset EFLAGS */
	pushl   $0
	popf

	/* Push the pointer to the Multiboot information structure. */
	pushl   %ebx
	/* Push the magic value. */
	pushl   %eax

	/* Now enter the C main function... */
	call    EXT_C(init)

	/* Halt. */
	pushl   $halt_message
	call    EXT_C(printf)

loop:
	hlt
	jmp     loop

halt_message:
	.asciz  "HCF"

	/* Our stack area. */
	.comm   stack, STACK_SIZE
