#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>

#define KBD_IOCTL_TEST _IOW(0, 0, struct kbd_action)
#define KBD_IOCTL_READKEY _IOR(0, 1, struct kbd_action)

int main(int argc, char const * argv [])
{
	//
	// Test ioctl
	//
	struct kbd_action
	{
		char key;
		int status;
	} ioctl_test;
	ioctl_test.status = 10;
	ioctl_test.key = 'a';

	int fd = open("/proc/kbd_test", O_RDONLY);
	ioctl(fd, KBD_IOCTL_TEST, &ioctl_test);
	close(fd);

	//
	// Test reading keys now
	//
	char c;
	int i = 0;
	int str_size = 4;
	char * str = malloc(sizeof(char) * str_size);
	while ((c = kbd_busywait_getchar()) != '\n')
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

	printf(str);
	return 0;
}
