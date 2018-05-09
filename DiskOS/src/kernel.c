#include <sys/multiboot.h>
#include <timer.h>
#include <kernel.h>
#include <kvideo.h>
#include <kmalloc.h>
#include <interrupt.h>
#include <ramdisk.h>
#include <sys/sched.h>


// #define ENABLE_STATEFUL_CR
#ifdef ENABLE_STATEFUL_CR
	#include <stateful_cr.h>
#endif

#define TEST_RD
#ifdef TEST_RD
	void * run_tests(void *);	
#endif


/* Check if the bit BIT in FLAGS is set. */
#define CHECK_FLAG(flags,bit) ((flags) & (1 << (bit)))

/* Check if MAGIC is valid and print the Multiboot information structure
pointed by ADDR. */
void
kmain(unsigned long magic, unsigned long addr)
{
	multiboot_info_t *mbi;

	/* Clear the screen. */
	cls();
	
	/* Am I booted by a Multiboot-compliant boot loader? */
	if (magic != MULTIBOOT_BOOTLOADER_MAGIC)
	{
		printf ("Invalid magic number: 0x%x\n", (unsigned) magic);
		return;
	}

	/* Set MBI to the address of the Multiboot information structure. */
	mbi = (multiboot_info_t *) addr;

	/* MULTIBOOT CHECKS */
	multiboot_flagscheck(mbi);

	/* init KERNEL MALLOC */
	printf("Multiboot flags check done.\n");


	/* setup IDT */
	printf("IDT setup ... ");
	init_idt();
	printf("done.\n");


	/* init KERNEL PIT */
	printf("PIT init ... ");
	init_pit();
	printf("done.\n");


	/* init KERNEL PIC */
	printf("PIC init ... ");
	init_pic();
	printf("done.\n");


	/* init KERNEL MALLOC */
	printf("kernel malloc init ... ");
	init_kmalloc(mbi);
	printf("done.\n");


	/* init SCHEDULER */
	printf("sched init ... ");
	init_sched();
	printf("done.\n");

	cls();
	print_banner();
	
	/* START STSCKFUL COROUTINES */
	#ifdef ENABLE_STATEFUL_CR
		cls();
		printf("registering stateful coroutines ... ");
		stateful_cr_register_routines();
		printf("done.\n");

		/* START SCHED */
		printf("starting preemptive scheduler ...\n");
	#endif

		/* run rd tests */
	#ifdef TEST_RD
		/* init RAM DISK */
		printf("ram disk init ... ");
		void * ramdisk_base_addr = kmalloc(UFS_DISK_SIZE);
		init_rdisk(ramdisk_base_addr);
		printf("done.\n");

		printf("starting tests...\n");
		run_tests(NULL);
	#endif

	/* set interrupt flag and then loop here */
	sti();
	noploop();
}


void
multiboot_flagscheck(multiboot_info_t * mbi)
{
	/* Print out the flags. */
	printf ("flags = 0x%x\n", (unsigned) mbi->flags);

	/* Are mem_* valid? */
	if (CHECK_FLAG (mbi->flags, 0))
		printf ("Total System Memory: Lower = %uKB, Upper = %uKB\n",
			(unsigned) mbi->mem_lower, (unsigned) mbi->mem_upper);

	/* Is boot_device valid? */
	if (CHECK_FLAG (mbi->flags, 1))
		printf ("boot_device = 0x%x\n", (unsigned) mbi->boot_device);

	/* Is the command line passed? */
	if (CHECK_FLAG (mbi->flags, 2))
		printf ("cmdline = %s\n", (char *) mbi->cmdline);

	/* SANITY CHECKS*/
	/* Are mods_* valid? */
	if (CHECK_FLAG (mbi->flags, 3))
	{
		multiboot_module_t *mod;
		int i;

		printf ("mods_count = %d, mods_addr = 0x%x\n",
			(int) mbi->mods_count, (int) mbi->mods_addr);
		
		for (i = 0, mod = (multiboot_module_t *) mbi->mods_addr;
			i < mbi->mods_count;
			i++, mod++)
		{
			printf (" mod_start = 0x%x, mod_end = 0x%x, cmdline = %s\n",
				(unsigned) mod->mod_start,
				(unsigned) mod->mod_end,
				(char *) mod->cmdline);
		}
	}

	/* Bits 4 and 5 are mutually exclusive! */
	if (CHECK_FLAG (mbi->flags, 4) && CHECK_FLAG (mbi->flags, 5))
	{
		printf ("Both bits 4 and 5 are set.\n");
		return;
	}

	/* Is the symbol table of a.out valid? */
	if (CHECK_FLAG (mbi->flags, 4))
	{
		multiboot_aout_symbol_table_t *multiboot_aout_sym = &(mbi->u.aout_sym);

		printf ("multiboot_aout_symbol_table: tabsize = 0x%0x, "
			"strsize = 0x%x, addr = 0x%x\n",
			(unsigned) multiboot_aout_sym->tabsize,
			(unsigned) multiboot_aout_sym->strsize,
			(unsigned) multiboot_aout_sym->addr);
	}

	/* Is the section header table of ELF valid? */
	if (CHECK_FLAG (mbi->flags, 5))
	{
		multiboot_elf_section_header_table_t *multiboot_elf_sec = &(mbi->u.elf_sec);

		printf ("multiboot_elf_sec: num = %u, size = 0x%x,"
			" addr = 0x%x, shndx = 0x%x\n",
			(unsigned) multiboot_elf_sec->num, (unsigned) multiboot_elf_sec->size,
			(unsigned) multiboot_elf_sec->addr, (unsigned) multiboot_elf_sec->shndx);
	}

	/* Are mmap_* valid? */
	if (CHECK_FLAG (mbi->flags, 6))
	{
		multiboot_memory_map_t *mmap;

		printf ("mmap_addr = 0x%x, mmap_length = 0x%x\n",
			(unsigned) mbi->mmap_addr, (unsigned) mbi->mmap_length);
		
		for (mmap = (multiboot_memory_map_t *) mbi->mmap_addr;
			(unsigned long) mmap < mbi->mmap_addr + mbi->mmap_length;
			mmap = (multiboot_memory_map_t *) ((unsigned long) mmap
				+ mmap->size + sizeof (mmap->size)))
		{
			printf (" size = 0x%x, base = 0x%x%08x,"
				" length = 0x%x%08x, type = 0x%x\n",
				(unsigned) mmap->size,
				(unsigned) (mmap->addr >> 32),
				(unsigned) (mmap->addr & 0xffffffff),
				(unsigned) (mmap->len >> 32),
				(unsigned) (mmap->len & 0xffffffff),
				(unsigned) mmap->type);
		}
	}
}


void
print_banner(void)
{
	printf(" __  __ ______ __  __ ______ ____   _____ \n");
	printf("|  \\/  |  ____|  \\/  |  ____/ __ \\ / ____|\n");
	printf("| \\  / | |__  | \\  / | |__ | |  | | (___  \n");
	printf("| |\\/| |  __| | |\\/| |  __|| |  | |\\___  \\\n");
	printf("| |  | | |____| |  | | |___| |__| |____) |\n");
	printf("|_|  |_|______|_|  |_|______\\____/|_____/ \n");
}

void
noploop(void)
{
	while(true)
		__asm__ volatile("nop" : :);
}
