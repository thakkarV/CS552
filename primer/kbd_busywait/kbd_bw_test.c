#include <stdlib.h>

int main(int argc, char const *argv[])
{
	//
	// Test ioctl
	//
	struct ioctl_test_t
	{
		int field1;
		char field2;
	} ioctl_test;

	int fd = open("/proc/kbd_test", O_RDONLY);

	ioctl_test.field1 = 10;
	ioctl_test.field2 = 'a';

	ioctl(fd, IOCTL_TEST, &ioctl_test);

	//
	// Test reading keys now
	//
	int i = 0;
	int str_size = 4;
	char * str = malloc(sizeof(char) * str_size);
	while ((c = kbd_busywait_getchar()) != '\n')
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
