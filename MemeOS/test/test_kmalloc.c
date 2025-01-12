#include <kmalloc>
#include <kvideo>
#include <types>

void
test_kmalloc(void)
{

	int ** mem = kmalloc(4 * sizeof(int *));
	if (!mem)
	{
		printf("malloc returned null\n");
	}
	else
	{

		printf("pointer from malloc = 0x%x\n", mem);
		*mem = 0x41414141;
		printf("actual val at ptr = 0x%x | expetced = 0x%x\n", *mem, mbi);
		kfree(mem);

		// try again
		mem = kmalloc(4 * sizeof(int *));
		printf("mem pointer = 0x%x\n", mem);
		*mem = 0xdeadbeef;
		printf("actual val at ptr = 0x%x | expetced = 0x%x\n", *mem, 0xdeadbeef);

		// try again
		int **  mem2 = kmalloc(16 * sizeof(int *));
		printf("mem2 pointer = 0x%x\n", mem2);
		*mem2 = 0xbeef;
		printf("actual val at ptr = 0x%x | expetced = 0x%x\n", *mem2, 0xbeef);
		
		mem = kmalloc(4 * sizeof(int *));
		printf("mem pointer = 0x%x\n", mem);
		*mem = 0xdead;
		printf("actual val at ptr = 0x%x | expetced = 0x%x\n", *mem, 0xdead);

		kfree(mem2);
		kfree(mem);
	}
}