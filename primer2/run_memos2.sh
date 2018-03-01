#!/bin/bash

KERNFILE="gnu_example"
ASMFILE="gnu_example"
LDFILE="memos2"
IMGFILE="memos2img"
MNTDEST="/mnt/C"

# gcc --32 $ASMFILE.S -o $ASMFILE.o
gcc $ASMFILE.S $KERNFILE.c -o $KERNFILE.o -std=gnu99 -ffreestanding -m32 -nostdlib
gcc -T $LDFILE.ld -o memos2 -nostdlib -ffreestanding $KERNFILE.o -lgcc

mount ./$IMGFILE.img $MNTDEST -t ext2 -o loop,offset=32256
cp memos2 $MNTDEST/boot
umount $MNTDEST

qemu-system-i386 -hda $IMGFILE.img
