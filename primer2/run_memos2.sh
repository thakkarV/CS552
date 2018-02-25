#!/bin/bash

ASFILE="memos2"
LDFILE="memos2"
IMGFILE="memos2img"
MNTDEST="/mnt/C"

as --32 $ASFILE.s -o $ASFILE.o
ld -T $LDFILE.ld $ASFILE.o -o $ASFILE
mount ./$IMGFILE.img $MNTDEST -t ext2 -o loop,offset=32256
cp memos2 $MNTDEST/boot
unmount $MNTDEST
qemu-system-i386 -hda $IMGFILE
