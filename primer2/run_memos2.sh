#!/bin/bash

KERNFILE="kernel"
ASFILE="memos2"
LDFILE="memos2"
IMGFILE="memos2img"
MNTDEST="/mnt/C"

gcc -c $KERNFILE.c -o $KERNFILE.o -std=gnu99 -ffreestanding
as --32 $ASFILE.S -o $ASFILE.o
ld -T $LDFILE.ld $ASFILE.o $KERNFILE.o -o memos2 -nostdlib
mount ./$IMGFILE.img $MNTDEST -t ext2 -o loop,offset=32256
cp memos2 $MNTDEST/boot
umount $MNTDEST
qemu-system-i386 -hda $IMGFILE.img
