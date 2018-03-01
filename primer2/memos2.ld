OUTPUT_FORMAT("elf32-i386")
OUTPUT_ARCH(i386)
ENTRY(_start)

SECTIONS
{
    /* start at 1MB i.e. high mem */
	. = 1M;

	/* place multiboot header first */

	/* block and align do essentially the same thing, but lets use both for the
	   sake of backwards compatibility */
	.text BLOCK(4K) : ALIGN(4K)
	{
		/* multiboot must go at the very beginning */
		/* *(.multiboot) */
		*(.text)
	}

	/* Read-write data (uninitialized) and stack */
	.bss BLOCK(4K) : ALIGN(4K)
	{
		*(COMMON)
		*(.bss)
	}

	/* Read-only data */
	.rodata BLOCK(4K) : ALIGN(4K)
	{
		*(.rodata)
	}

	/* Read-write data (initialized) */
	.data BLOCK(4K) : ALIGN(4K)
	{
		*(.data)
	}
	.ehframe : { *(.ehframe) }
}
