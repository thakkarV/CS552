	.global _start
	.code16

_start:
	# setup stack
	movw $0x9000, %ax
	movw %ax, %ss
	xorw %sp, %sp
	
	movw $0x7C0, %dx
	movw %dx, %ds

# print out welcome message
	leaw msg, %si
	movw len_msg, %cx
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
	movw $0xE801, %ax
	int 0x15

	# ax holds kB between 1-16 MB
	# bx holds number of 64k pages above 16 MB 

	# account for the lower 1 meg of bios memory
	addw $0x400, %ax

	# copy everything over
	movw %ax, %cx
	movw %bx, %dx

	# DO FANCY TRICKS HERE

	shr $8, %ax
	call print
	movw %cx, %ax
	andb $0xFF, %ax
	call print

# print out units
	leaw memunit, %si
	movw len_memunit, %cx
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
