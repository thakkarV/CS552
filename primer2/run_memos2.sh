#!/bin/bash

KERNFILE="kernel"
ASFILE="memos2"
LDFILE="memos2"
IMGFILE="memos2img"
MNTDEST="/mnt/C"

i686-elf-as $ASFILE.S -o $ASFILE.o
i686-elf-gcc $KERNFILE.c -o $KERNFILE.o -std=gnu99 -ffreestanding
i686-elf-gcc -T $LDFILE.ld -o memos2 -nostdlib -ffreestanding $ASFILE.o $KERNFILE.o -lgcc

mount ./$IMGFILE.img $MNTDEST -t ext2 -o loop,offset=32256
cp memos2 $MNTDEST/boot
umount $MNTDEST

qemu-system-i386 -hda $IMGFILE.img
