#ifndef KERNEL_INIT
#define KERNEL_INIT

#include "multiboot.h"

void init (unsigned long magic, unsigned long addr);
void multiboot_flagscheck(multiboot_info_t * mbi);
void print_banner(void);
void noploop(void);

#endif
