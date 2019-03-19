CC = gcc
# no-pie disables position independent code, here we use it to
# get rid of the pc thunk calls
CFLAGS = -I./include -m32 -march=i386 -fno-builtin \
		 -fno-stack-protector -fno-strict-aliasing \
		 -fno-delete-null-pointer-checks -nostdinc \
		 -fno-pie -g -Wall -std=c99
CPPFLAGS = -Wa,--32 -MMD
CMDMACROS =

# define project directory structure
SRCDIR    = src
ASMDIR    = asm
HEADERDIR = include
OBJDIR    = obj
BINDIR    = bin
LDFILE    = kernel.ld

TARGET    = MemeOS
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
	$(CC) $(CFLAGS) $(CMDMACROS) -c -o $@ $<

$(OBJDIR)/%.S.o: $(ASMDIR)/%.S
	$(CC) $(CFLAGS) $(CMDMACROS) $(CPPFLAGS) -c -o $@ $<


install: $(BINDIR)/$(TARGET)
	cp $(BINDIR)/$(TARGET) $(MNT_POINT)/boot
	sync

clean:
	-rm -f $(OBJS) $(TARGET) *~

rbin:
	qemu-system-i386 -kernel $(BINDIR)/$(TARGET)

rdisk:
	qemu-system-i386 ./MemeOS.img

remake: clean all

rrb: clean all rbin

rrd: clean all install rdisk

# this is the same as rrb
# but allows attaching gdb through tcp port 9000
# -s can be used as a shorthand for -gdb tcp::1234 instead
# -S flag halts the execution until gdb run command is given
# -d {cpu,exec,in_asm,cpu_reset,int} is used for dumping registers
# at certain hardware events
# after this, the symbols must still be loaded with the file command
debug: clean all
	qemu-system-i386 -kernel $(BINDIR)/$(TARGET) \
		-gdb tcp::9000 -S
