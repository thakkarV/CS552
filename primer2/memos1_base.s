	.global _start
	.code16

_start:
	# do setup stuff here
	movw $0x9000, %ax
	movw %ax, %ss
	xorw %sp, %sp 		# FIXME: why set sp to 0?

first_e820:

	# FIXME: is this correct?
	# set ES:DI to destination buffer
	movw %ss, %ax
	movw %ax, %es
	xorw %di, %di

	# clear EBX
	xor %ebx, %ebx

	# set EDX to the magic number 0x534D4150
	mov $0x534D4150, %edx

	# set EAX to 0xE820, ECX to 24 bytes
	mov $0xE820, %eax
	mov $24, %ecx
	int $0x15

	# first call failed if carry==1 or eax!=0x534D4150
	jc failed
	mov $0x534D4150, %edx
	cmp %eax, %edx
	jne failed

print_section:

	# skip if 0 length entry
	jcxz sub_e820

	# TODO: first byte = lowest addr or highest addr on stack?

sub_e820:
	
	# eax and ecx must be set again for subsequent e820 calls
	mov $0xE820, %eax
	mov $24, %ecx
	int $0x15

	# end of list if carry==1 or ebx==0
	jc end_of_list
	test %ebx, %ebx
	je end_of_list

	# print current result then call e820 again for next section
	jmp print_section

end_of_list:
	# print successful read message for user
	hlt

failed:
	# print error message for user
	hlt
