	.global _start
	.code16

_start:
	# do setup stuff here
	mov $0x9000, %ax
	mov %ax, %ss
	xor %sp, %sp

	# print welcome message
	lea msg_welcome, %si
	mov len_msg_welcome, %cx
	call print_str

	# TODO: Print total system memory here in MBs

	# FIXME: is this correct?
	# set ES:DI to destination buffer
	# we set this up at 0x7E00, guaranteed free section until 0x0007FFFF
	mov $0x7E0, %ax
	mov %ax, %es
	mov $0x0, %di

	# clear EBX
	xor %ebx, %ebx

	# store total count of entries in bp
	xor %bp, %bp

	# start E820
	jmp do_e820	


do_e820:
	# set EAX to 0xE820, ECX to 24 bytes
	mov $0xE820, %eax
	mov $24, %ecx
	# set EDX to the magic number 0x534D4150
	mov $0x534D4150, %edx

	# raise interrupt for getting next section
	int $0x15

	# Error if eax!=0x534D4150 or carry==1
	mov $0x534D4150, %eax
	cmp %eax, %edx
	jne err
	jc err

	# skip if 0 length entry
	# eax gets the base address of the memory
	# ecx gets the length
	# or the length with base of see if length is 0
	# if true, skip this entry, else go to print statements
	mov %es:(%di + 2), %eax
	mov %es:(%di + 6), %ecx
	or %eax, %ecx
	jz do_e820

	# if error checks pass, print section type
	jmp print_section


print_section:
	# print addr string first
	lea msg_addr_range, %si
	mov len_msg_addr_range, %cx
	call print

	# print memory range
	# TODO: start

	# colon
	movb $0x3A, %al
	movb $0x0E, %ah
	int $0x10

	# TODO: end

	# print status string
	lea msg_status, %si
	mov len_msg_status, %cx
	call print_str

	# assume this entry is always 20 bytes long - ACPI 3.x unsupported
	mov %es:(%di + 16), %eax

	# jump table for printing the types
.type1
	cmp %eax, $1
	jne .type2
	lea type_usable, %si
	mov len_type_usable, %cx
	call print_str
.type2
	cmp %eax, $2
	jne .type3
	lea type_reserved, %si
	mov len_type_reserved, %cx
	call print_str
.type3
	cmp %eax, $3
	jne .type4
	lea type_acpi_reclaimable, %si
	mov len_type_acpi_reclaimable, %cx
	call print_str
.type4
	cmp %eax, $4
	jne .type5
	lea type_acpi_nvs, %si
	mov len_type_acpi_nvs, %cx
	call print_str
.type5
	cmp %eax, $5
	jne err
	lea type_badmem, %si
	mov len_type_badmem, %cx
	call print_str

	# increment es:di by 20, which is the size of the entry
	# FIXME: make sure this is 20/24
	add $20, %di
	# increment total number of entries seen
	inc %bp
	
	# End of list if EBX is 0
	test %ebx, %ebx
	jz end
	# else, go to the next memory section
	jmp do_e820


end:
	lea msg_done, %si
	mov len_msg_status, %cx
	call print_str
	hlt


err:
	lea msg_err, %si
	mov len_msg_err, %cx
	call print_str
	hlt


print_str:	
	# the string to be printed is passed through si
	# length of the string is passed through cx
	push %ax
1:
	lodsb
	movb $0x0E, %ah
	int $0x10
	loop 1b

	pop %ax
	ret


# ####### #
# STRINGS #
# ####### #
msg_welcome: .ascii "MemOS: Welcome *** System Memory is: "
len_msg_welcome: .word . - msg_welcome

msg_units: .asciz "MB"
len_msg_unit: .word . - msg_units

msg_addr_range: .ascii "Address range ["
len_msg_addr_range: .word . - msg_addr_range

msg_status: .ascii "] status: "
len_msg_status: .word . - msg_status

# Memory Type Strings
type_usable: .asciz "USABLE RAM"
len_type_usable: .word . - type_usable

type_reserved: .asciz "RESERVED"
len_type_reserved: .word . - type_reserved

type_acpi_recliamable: .asciz "ACPI RECLAIMABLE MEMORY"
len_type_acpi_recliamable: .word . - type_acpi_recliamable

type_acpi_nvs: .asciz "ACPI NVS MEMORY"
len_type_acpi_nvs: .word . - type_acpi_nvs

type_badmem: .asciz "BAD MEMORY"
len_type_badmem: .word . - type_badmem

msg_done: .asciz "Done"
len_msg_done: .word . - msg_done

msg_err: .asciz "Error"
len_msg_err: .word . - msg_err

# ######## #
# MARK MBR #
# ######## #
	.org 0x1FE
	.byte 0x55
	.byte 0xAA
