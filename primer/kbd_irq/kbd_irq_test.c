#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>

int main(int argc, char const * argv [])
{
	//
	// Test reading keys now
	//
	char c;
	int i = 0;
	int str_size = 4;
	char * str = malloc(sizeof(char) * str_size);
	while ((c = kbd_irq_getchar()) != '\n')
	{
		str[i++] = c;
		if (i >= str_size)
		{
			str_size *= 2;
			str = realloc(str, str_size);
			if (!str)
			{
				printf("realloc error\n");
				exit(1);
			}
		}
	}

	printf("%s", str);
	return 0;
}
