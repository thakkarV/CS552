# TODO: try if switch saves anything
# TODO: add back skuip 0 length

	.global _start
	.code16

_start:
	# SS starts at 0x7E00
	mov $0x7E0, %ax
	mov %ax, %ss

	# SP init at 0x20000, giving 8K of stack space
	mov $0x2000, %sp

	# print welcome message
	lea msg_welcome, %si
	mov len_msg_welcome, %cx
	call print_str

	# set dest buffer ES:DI at 0x20000, free space till 0x7FFFF
	mov $0x2000, %ax
	mov %ax, %es
	xor %di, %di

	# while doing E820, keep count of total regions in BP
	xor %bp, %bp
	
	# clear EBX for E820
	xorl %ebx, %ebx

do_e820:
	# EAX = 0xE820
	# EBX = 0 (BIOS book-keeping for E820 calls, do not touch afterwards)
	# ECX = 0d24 (ask for 24 bytes in the entry)
	# EDX = 0x534D4150
	movl $0x534D4150, %edx
	movl $24, %ecx
	mov $0xE820, %ax
	int $0x15

	# Done or Error if eax!=0x534D4150 or carry==1
	jc end
	movl $0x534D4150, %eax
	cmpl %eax, %edx
	jne end

	# increment numebr of memory regions for print call counter
	inc %bp
	
	# check if last entry, if yes, jmp print
	testl %ebx, %ebx
	jz print_region

	# increment DI by list entry size stored in CL (i.e. CX low 8 bits)
	add $24, %di

	jmp do_e820


print_region:
	# TODO: pint total memory here first

	# print units
	lea msg_units, %si
	mov len_msg_units, %cx
	call print_str

	# reset di to 0 so we can restart indexing into buffer for printouts
	xor %di, %di

.print_start:
	# print addr string first
	lea msg_addr_range, %si
	mov len_msg_addr_range, %cx
	call print_str

	# print region start addr
	# eax gets the base address of the memory
	movl %es:0(%di), %eax
	call print_num

	# colon
	push %ax
	movb $0x3A, %al
	movb $0x0E, %ah
	int $0x10
	pop %ax

	# print region end addr
	# ECX stores the length, end adrr = begin + len - 1
	movl %es:8(%di), %ecx
 	addl %ecx, %eax
	subl $0x1, %eax
	call print_num

	# print region type
	# assume this entry is always 20 bytes long - ACPI 3.x unsupported
	movl %es:16(%di), %eax

.type1:
	cmp $1, %ax
	jne .type2
	lea type_usable, %si
	mov len_type_usable, %cx
	jmp .return
.type2:
	cmp $2, %ax
	jne .type3
	lea type_reserved, %si
	mov len_type_reserved, %cx
	jmp .return
.type3:
	cmp $3, %ax
	jne .type4
	lea type_acpi_reclaimable, %si
	mov len_type_acpi_reclaimable, %cx
	jmp .return
.type4:
	cmp $4, %ax
	jne .type5
	lea type_acpi_nvs, %si
	mov len_type_acpi_nvs, %cx
	jmp .return
.type5:
	cmp $5, %ax
	jne end
	lea type_badmem, %si
	mov len_type_badmem, %cx
.return:
	call print_str

	# prep for next section
	dec %bp
	add $24, %di
	cmp $0, %bp
	jnz .print_start
	jmp end


print_str:
	# the string to be printed is passed through si
	# length of the string is passed through cx
	pushl %eax
1:
	lodsb
	movb $0x0E, %ah
	int $0x10
	loop 1b

	popl %eax
	ret

# prints the 32 bit number passed through in EAX in decimal
print_num:
	# save regs
	pushl %ebx
	pushl %ecx
	pushl %edx

	movl $10, %ebx
	mov $10, %cx

.loop_div:
	# dx needs to be set to 0 before div call
	xorl %edx, %edx
	# devide EAX by BX
	# stores quotient in EAX
	# stores remainder in DX
	divl %ebx
	# store modulo of division onto the stack
	pushl %edx
	loop .loop_div

	mov $10, %cx
.loop_print:
	popl %edx
	# add 48 as the ASCII offset
	add $48, %dx
	movb %dl, %al
	movb $0x0E, %ah
	int $0x10
	loop .loop_print
	# print dx as a ascii char

	popl %edx
	popl %ecx
	popl %ebx
	ret

end:
.stop:
	hlt
	jmp .stop


# ####### #
# STRINGS #
# ####### #
msg_welcome: .asciz "MemOS: Welcome *** System Memory is: "
len_msg_welcome: .word . - msg_welcome

msg_units: .asciz "MB\015\012"
len_msg_units: .word . - msg_units

msg_addr_range: .asciz "Address range ["
len_msg_addr_range: .word . - msg_addr_range

msg_status: .asciz " status: "
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

# ######## #
# MARK MBR #
# ######## #
	.org 0x1FE
	.byte 0x55
	.byte 0xAA
