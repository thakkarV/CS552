#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#define KBD_IOCTL_READKEY _IOR(0, 1, char)

char kbd_busywait_getchar(void)
{
	char c;
	int fd = open("/proc/kbd_bw", O_RDONLY);
	ioctl(fd, KBD_IOCTL_READKEY, &c);
	close(fd);
	return c;
}
