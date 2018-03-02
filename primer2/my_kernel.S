#include "multiboot.h"

#define MULTIBOOT_HEADER_FLAGS MULTIBOOT_PAGE_ALIGN | MULTIBOOT_MEMORY_INFO | MULTIBOOT_VIDEO_MODE

	.text

	.global _start

_start:
	jmp real_start

	/* Align 32 bits boundary. */
	.align 4


multiboot_header:
	/* magic */
	.long	MULTIBOOT_HEADER_MAGIC
	/* flags */
	.long	MULTIBOOT_HEADER_FLAGS
	/* checksum */
	.long	-(MULTIBOOT_HEADER_MAGIC + MULTIBOOT_HEADER_FLAGS)

real_start:
	call	cmain

	pushl	$halt_message
	call	EXT_C(printf)

lp:
	hlt
	jmp lp

halt_message:
	.asciz "Halted."

	/* Our stack area. */
	.comm  stack, STACK_SIZE
