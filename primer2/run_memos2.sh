#!/bin/bash

KERNFILE="gnu_example"
ASMFILE="gnu_example"
LDFILE="memos2"
IMGFILE="disk"
MNTDEST="/mnt/D"

# gcc --32 $ASMFILE.S -o $ASMFILE.o
gcc -march=i386 $ASMFILE.S $KERNFILE.c -o $KERNFILE.o -std=gnu99 -ffreestanding -m32 -nostdlib -lgcc -Wall -Wextra
gcc -T $LDFILE.ld -o memos2 -nostdlib -ffreestanding $KERNFILE.o -lgcc -Wall -Wextra

mount /root/host/CS552/primer2/$IMGFILE.img $MNTDEST -t ext2 -o loop,offset=32256
cp /root/host/CS552/primer2/memos2 $MNTDEST/boot
sync
qemu-system-i386 -hda $IMGFILE.img
