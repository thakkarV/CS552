#include <stdlib.h>

int main(int argc, char const *argv[])
{
	int i = 0;
	int str_size = 4;
	char * str = malloc(sizeof(char) * str_size);
	while ((c = my_getchar()) != '\n')
	{
		str[i++]
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

	printf(str);
	return 0;
}