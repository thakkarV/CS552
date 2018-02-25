	.global _start
	.code16

_start:
	# DS starts at 0x7C00
	mov $0x7C0, %ax
	mov %ax, %ds

	# SS starts at 0x7E00
	mov $0x7E0, %ax
	mov %ax, %ss

	# SP init at 0x20000, giving 8K of stack space
	mov $0x2000, %sp

	# print welcome message
	lea msg_welcome, %si
	mov len_msg_welcome, %cx
	call print_str

	# TODO: Print total system memory here in MBs

	# print units
	lea msg_units, %si
	mov len_msg_units, %cx
	call print_str

	# set dest buffer ES:DI at 0x30000, free space till 0x7FFFF
	mov $0x3000, %ax
	mov %ax, %es
	xor %di, %di


first_e820:
	# clear EBX
	xor %ebx, %ebx

	# EAX = 0xE820
	# EBX = 0 (BIOS book-keeping for E820 calls, do not touch afterwards)
	# ECX = 0d24 (ask for 24 bytes in the entry)
	# EDX = 0x534D4150
	mov $0x534D4150, %edx
	mov $24, %ecx
	mov $0xE820, %eax
	int $0x15

	# Error if eax!=0x534D4150 or carry==1 (only for the first call)
	mov $0x534D4150, %eax
	cmp %eax, %edx
	jne err
	jc err

	call print_region


loop_e820:
	mov $0xE820, %eax
	mov $24, %ecx
	mov $0x534D4150, %edx
	int $0x15

	# Error if edx!=0x534D4150
	mov $0x534D4150, %eax
	cmp %eax, %edx
	jne err

	# skip if 0 length entry
	# eax gets the base address of the memory
	# ecx gets the length
	# or the length with base of see if length is 0
	# if true, skip this entry, else go to print statements
	mov %es:2(%di), %eax
	mov %es:6(%di), %ecx
	or %eax, %ecx
	jz loop_e820

	# if error checks pass, print section type
	call print_region

	# End of list if EBX is 0, or if carry flag is set
	jc end
	test %ebx, %ebx
	jz end

	# increment DI by list entry size stored in CL (i.e. CX low 8 bits)
	andw $0xFF, %cx
	addw %cx, %di

	# Otherwise go to the next region
	jmp loop_e820


print_region:
	push %ecx
	push %eax

	# print addr string first
	lea msg_addr_range, %si
	mov len_msg_addr_range, %cx
	call print_str

	# print memory range
	# TODO: print region start

	# colon
	movb $0x3A, %al
	movb $0x0E, %ah
	int $0x10

	# TODO: print region end

	# print status string
	lea msg_status, %si
	mov len_msg_status, %cx
	call print_str

	# assume this entry is always 20 bytes long - ACPI 3.x unsupported
	mov %es:16(%di), %eax

	# jump table for printing the types
.type1:
	cmp $1, %eax
	jne .type2
	lea type_usable, %si
	mov len_type_usable, %cx
	jmp .return
.type2:
	cmp $2, %eax
	jne .type3
	lea type_reserved, %si
	mov len_type_reserved, %cx
	jmp .return
.type3:
	cmp $3, %eax
	jne .type4
	lea type_acpi_reclaimable, %si
	mov len_type_acpi_reclaimable, %cx
	jmp .return
.type4:
	cmp $4, %eax
	jne .type5
	lea type_acpi_nvs, %si
	mov len_type_acpi_nvs, %cx
	jmp .return
.type5:
	cmp $5, %eax
	jne err
	lea type_badmem, %si
	mov len_type_badmem, %cx
.return:
	# else, go to the next memory section
	call print_str
	pop %eax
	pop %ecx
	ret


end:
	# lea msg_done, %si
	# mov len_msg_done, %cx
	# call print_str
	sysexit


err:
	lea msg_err, %si
	mov len_msg_err, %cx
	call print_str
	sysexit


print_str:
	# the string to be printed is passed through si
	# length of the string is passed through cx
	push %eax
1:
	lodsb
	movb $0x0E, %ah
	int $0x10
	loop 1b

	pop %eax
	ret

# prints the 32 bit number passed through in EAX in decimal
print_num:
	# save regs
	push %ebx
	push %ecx
	push %edx

	# load 10 and 32
	mov $10, %bx
	mov $16, %cx
.loop_div:
	# devide EAX by BX
	# stores quotient in EAX
	# stores remainder in DX
	div %bx
	# store modulo of division onto the stack
	push %dx
	loop .loop_div

	mov $16, %cx
.loop_print:
	pop %dx
	# add 48 as the ASCII offset
	add $48, %dl
	mov %dl, %al
	mov $0x0E, %ah
	int $0x10
	loop .loop_print
	# print dx as a ascii char

	pop %edx
	pop %ecx
	pop %ebx
	ret

# ####### #
# STRINGS #
# ####### #
msg_welcome: .asciz "MemOS: Welcome *** System Memory is: "
len_msg_welcome: .word . - msg_welcome

msg_units: .asciz "MB\015\012"
len_msg_units: .word . - msg_units

msg_addr_range: .asciz "Address range ["
len_msg_addr_range: .word . - msg_addr_range

msg_status: .asciz "] status: "
len_msg_status: .word . - msg_status

# Memory Type Strings
type_usable: .asciz "USABLE RAM\015\012"
len_type_usable: .word . - type_usable

type_reserved: .asciz "RESERVED\015\012"
len_type_reserved: .word . - type_reserved

type_acpi_reclaimable: .asciz "ACPI RECLAIMABLE MEMORY\015\012"
len_type_acpi_reclaimable: .word . - type_acpi_reclaimable

type_acpi_nvs: .asciz "ACPI NVS MEMORY\015\012"
len_type_acpi_nvs: .word . - type_acpi_nvs

type_badmem: .asciz "BAD MEMORY\015\012"
len_type_badmem: .word . - type_badmem

# msg_done: .asciz "Done\015\012"
# len_msg_done: .word . - msg_done

msg_err: .asciz "Error\015\012"
len_msg_err: .word . - msg_err

# ######## #
# MARK MBR #
# ######## #
	.org 0x1FE
	.byte 0x55
	.byte 0xAA
