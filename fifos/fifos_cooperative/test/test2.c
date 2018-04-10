
#include <stdio.h>

typedef enum
{
	val1,
	val2
} testenum;

int main()
{
	printf("uint is %d", sizeof(unsigned int));
	printf("logn is %d", sizeof(long));
	printf("testenum is %d\n", sizeof(testenum));

	return 0;
}
