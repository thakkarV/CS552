	.global _start
	.code16

_start:
	# setup stack
	mov $0x9000, %ax
	mov %ax, %ss
	xorw %sp, %sp
	
	mov $0x7C0, %dx
	mov %dx, %ds

# print out welcome message
	leaw msg, %si
	mov len_msg, %cx
1:
	lodsb
	movb $0x0E, %ah
	int 0x10
	loop 1b

# write hex to screen
	movb $'0, %al
	movb $0x0E, %ah
	int 0x10
	movb $'x, %al
	movb %0x0E, %ah
	int 0x10

# probe memory
	mov $0xE820, %ax
	xorw %bx, %bx
	mov %ss, %es:(%di) # fugure out if this is correct
	# figure out how to set magic number for ebx
	mov $24, %cx #verify correctness

	int 0x15


# print out units
	leaw memunit, %si
	mov len_memunit, %cx
1:
	lodsb
	movb $0x0E, %ah
	int 0x10
	loop 1b

	jmp end


print:
	pushw %dx

# print MSB
	movb %al, %dl
	shrb $4, %al
	cmpb $10, %al
	jge 1f
	addb %48, %al
	jmp 2f
1:	addb $55, %al
2:	movb $0x0E, %ah
	int 0x10

# print LSB
	movb %dl, %al
	andb $0x0F, %al
	cmpb $10, %al
	jge 1f
1:	addb $55, %al
2:	movb $0x0E, %ah
	int 0x10

	popw %dx
	ret


msg: .ascii "MemOS: Wecome -- System Memory is: "
len_msg: .word . - msg


memunit: .ascii "KB"
len_memunit: .word . - memunit


end:
	hlt

	.org 0x1FE
	.byte 0x55
	.byte 0xAA
