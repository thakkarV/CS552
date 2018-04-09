#!/bin/bash

KERNFILE="memos2"
ASMFILE="memos2"
LDFILE="memos2"
IMGFILE="disk"
MNTDEST="/mnt/D"

# gcc --32 $ASMFILE.S -o $ASMFILE.o
gcc -march=i386 $ASMFILE.S  $KERNFILE.c -o $KERNFILE.o  -std=gnu99 -nostartfiles -ffreestanding -m32 -nostdlib -lgcc -Wall -Wextra
ld -T $LDFILE.ld -o $KERNFILE.bin $KERNFILE.o

qemu-system-i386 -kernel $KERNFILE.bin -m 32 -vnc  :0
