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
1:
	lodsb
	movb $0x0E, %ah
	int $0x10
	loop 1b

	# FIXME: is this correct?
	# set ES:DI to destination buffer
	# we set this up at 0x7E00, guaranteed free section until 0x0007FFFF
	mov $0x7E0, %ax
	mov %ax, %es
	mov $0x0, %di

	# clear EBX
	xor %ebx, %ebx

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

	# first call err if eax!=0x534D4150 or carry==1
	mov $0x534D4150, %edx
	cmp %eax, %edx
	jne err
	jc err

	# if error checks dpass, print section type
	jmp print_section


print_section:
	# skip if 0 length entry
	jcxz do_e820

	# TODO: first byte = lowest addr or highest addr on stack?
	

	# TODO: increment es:di by %cl bytes

	# finally, go to the next memory section
	jmp do_e820


end:
	# print successful read message for user
	hlt


err:
	call print_err
	# print error message for user
	hlt


print_err:
	lea msg_err, %si
	mov len_msg_err, %cx
1:
	lodsb
	movb $0x0E, %ah
	int $0x10
	loop 1b


msg_err: .ascii "Error"
len_msg_err: .word . - msg_err

msg_welcome: .ascii "Welcome to Memos-1\nMemory sections are:\n"
len_msg_welcome: .word . - msg_welcome

	.org 0x1FE
	.byte 0x55
	.byte 0xAA
