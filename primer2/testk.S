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

#define ASM_FILE        1
#include "multiboot.h"

/* The size of our stack (16KB). */
#define STACK_SIZE 0x4000

/* The flags for the Multiboot header. */
#ifdef __ELF__
# define AOUT_KLUDGE 0
#else
# define AOUT_KLUDGE MULTIBOOT_AOUT_KLUDGE
#endif
#define MULTIBOOT_HEADER_FLAGS MULTIBOOT_PAGE_ALIGN | MULTIBOOT_MEMORY_INFO

	.text

	.globl  start, _start
start:
_start:
	jmp     multiboot_entry

	/* Align 32 bits boundary. */

	/* Multiboot header. */
multiboot_header:
	.align  4
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
	.long 0
	.long 1024
	.long 768
	.long 32

stack_bottom:
    .skip 16384
stack_top:

multiboot_entry:
	/* Initialize the stack pointer. */
	movl    $(stack_top), %esp

	movl $0x07690748, 0xb8000
	movl $0x07690748, 0xb8004
	movl $0x07690748, 0xb8008
	movl $0x07690748, 0xb800C


	/* Reset EFLAGS. */
	# pushl   $0
	# popf

	/* Push the pointer to the Multiboot information structure. */
	# pushl   %ebx
	/* Push the magic value. */
	# pushl   %eax

	/* Now enter the C main function... */
	# call cmain

	# movl $halt_message, %eax
	# pushl %eax
	# call print_str
	/* Halt. */

spin:
	hlt
	jmp spin

halt_message:
	.asciz  "Halted."

	/* Our stack area. */
	# .comm   stack, STACK_SIZE
