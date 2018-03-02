/* boot.S - bootstrap the kernel */
/* Copyright (C) 1999, 2001, 2010  Free Software Foundation, Inc.
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#define ASM_FILE 1
#include "multiboot.h"


/* The size of our stack (16KB). */
#define STACK_SIZE 0x4000


#define MULTIBOOT_HEADER_FLAGS MULTIBOOT_PAGE_ALIGN | MULTIBOOT_MEMORY_INFO | MULTIBOOT_VIDEO_MODE

	.text

	.global _start
_start:
	jmp real_start

	/* Align 32 bits boundary. */
	.align  4

		/* Multiboot header. */
multiboot_header:
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
	.long   real_start
#else /* ! __ELF__ */
	.long   0
	.long   0
	.long   0
	.long   0
	.long   0
#endif /* __ELF__ */
	.long 0
	.long 1024
	.long 768
	.long 32

real_start:
	/* Initialize the stack pointer. */
	movl    $(stack + STACK_SIZE), %esp

	/* Reset EFLAGS. */
	pushl   $0
	popf

	/* Now enter the C main function... */
	call    cmain

	/* Halt. */
	pushl   $halt_message
	call    printf

lp:
	hlt
	jmp lp

halt_message:
	.asciz  "Halted."

	/* Our stack area. */
	.comm   stack, STACK_SIZE
