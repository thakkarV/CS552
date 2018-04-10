CC = gcc
CFLAGS = -I./include -m32 -march=i386 -fno-builtin -fno-stack-protector -fno-strict-aliasing -fno-delete-null-pointer-checks -nostdinc -g -Wall -std=c99
CPPFLAGS = -Wa,--32 -MMD

# define project directory structure
SRCDIR    = src
ASMDIR    = asm
HEADERDIR = include
OBJDIR    = obj
BINDIR    = bin
LDFILE    = kernel.ld

TARGET    = fifos
MNT_POINT = /mnt/C


# gather all files
HEADERS  := $(wildcard $(HEADERDIR)/*.h)
CSRC     := $(wildcard $(SRCDIR)/*.c)
ASMSRC   := $(wildcard $(ASMDIR)/*.S)

SRCOBJS  := $(CSRC:$(SRCDIR)/%.c=$(OBJDIR)/%.c.o)
ASMOBJS  := $(ASMSRC:$(ASMDIR)/%.S=$(OBJDIR)/%.S.o)
OBJS     := $(SRCOBJS) $(ASMOBJS)

all: $(BINDIR)/$(TARGET)

$(BINDIR)/$(TARGET): $(OBJS)
	$(LD) -m elf_i386 -T $(LDFILE) -o $@ $^

$(OBJDIR)/%.c.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJDIR)/%.S.o: $(ASMDIR)/%.S
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<


install: $(BINDIR)/$(TARGET)
	cp $(BINDIR)/$(TARGET) $(MNT_POINT)/boot
	sync

clean:
	-rm -f $(OBJS) $(TARGET) *~

rbin:
	qemu-system-i386 -kernel $(BINDIR)/$(TARGET) -d cpu,exec,in_asm,cpu_reset,int

rdisk:
	qemu-system-i386 ./fifos.img -d cpu,exec,in_asm,cpu_reset,int

remake: clean all

rrb: clean all rbin

rrd: clean all install rdisk
