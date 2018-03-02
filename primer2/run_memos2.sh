#!/bin/bash

KERNFILE="testk"
ASMFILE="testk"
LDFILE="memos2"
IMGFILE="disk"
MNTDEST="/mnt/D"

# gcc --32 $ASMFILE.S -o $ASMFILE.o
gcc -march=i386 $ASMFILE.S $KERNFILE.c -o $KERNFILE.o -std=gnu99 -ffreestanding -m32 -nostdlib -lgcc -Wall -Wextra
gcc -T $LDFILE.ld -o $KERNFILE.bin $KERNFILE.o -lgcc -Wall -Wextra -std=gnu99 -nostdlib -ffreestanding

mount /root/host/CS552/primer2/$IMGFILE.img $MNTDEST -t ext2 -o loop,offset=32256
cp /root/host/CS552/primer2/$KERNFILE.bin $MNTDEST/boot
sync
qemu-system-i386 -hda $IMGFILE.img
