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

	while ((c = kbd_irq_getchar()) != '\n')
	{
		printf("Got %c \n", c);
	}

	return 0;
}
