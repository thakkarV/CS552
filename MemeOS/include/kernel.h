#ifndef KERNEL_INIT
#define KERNEL_INIT

#include <sys/multiboot.h>

void kmain(unsigned long magic, unsigned long addr);
void *init(void *);
void multiboot_flagscheck(multiboot_info_t *mbi);
void print_banner(void);
void noploop(void);

#endif
