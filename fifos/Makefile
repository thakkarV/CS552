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

TARGET    = $(BINDIR)/fifos
MNT_POINT = /mnt/


# gather all files
HEADERS  := $(wildcard $(HEADERDIR)/*.h)
CSRC     := $(wildcard $(SRCDIR)/*.c)
ASMSRC   := $(wildcard $(ASMDIR)/*.S)

SRCOBJS  := $(CSRC:$(SRCDIR)/%.c=$(OBJDIR)/%.o)
ASMOBJS  := $(ASMSRC:$(ASMDIR)/%.S=$(OBJDIR)/%.o)
OBJS     := $(SRCOBJS) $(ASMOBJS)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(LD) -m elf_i386 -T $(LDFILE) -o $@ $^

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJDIR)/%.o: $(ASMDIR)/%.S
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<


install: $(TARGET)
	cp $(BINDIR)/$(TARGET) $(MNT_POINT)/boot
	sync

clean:
	-rm -f $(OBJS) $(TARGET) *~

run:
	qemu-system-i386 -kernel $(TARGET) -d cpu,exec,in_asm

remake: clean all

rr: clean all run
