#! /bin/bash

ASFILE="memos1"
LDFILE="memos1"
IMGFILE="memos1.img"
MEMS_SIZE=8
VNC_PORT=0

as --32 $ASFILE.S -o $ASFILE.o
ld -T $LDFILE.ld $ASFILE.o -o $ASFILE
dd bs=1 if=$ASFILE of=$IMGFILE count=512
qemu-system-i386 -hda $IMGFILE -m $MEMS_SIZE -vnc :$VNC_PORT
