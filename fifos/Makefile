CC = gcc
CFLAGS = -m32 -march=i386 -I. -fno-builtin -fno-stack-protector -fno-strict-aliasing -fno-delete-null-pointer-checks -nostdinc -I. -g -Wall -std=c99
CPPFLAGS = -Wa,--32 -MMD
OBJS = loader.o kernel.o kvideo.o kmalloc.o stdlib.o
PROGS = fifos
MNT_POINT=/mnt/

all: $(PROGS)

fifos: $(OBJS)
	$(LD) -m elf_i386 -T kernel.ld -I. -o $@ $^

%: %.c

install: $(PROGS)
	cp $(PROGS) $(MNT_POINT)/boot
	sync

clean:
	-rm *.o *.d $(PROGS) *~

run:
	qemu-system-i386 -kernel $(PROGS)

remake: clean all

-include *.d
