#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#define KBD_IOCTL_TEST _IOW(0, 0, struct kbd_action)
#define KBD_IOCTL_READKEY _IOR(0, 1, struct kbd_action)

char kbd_busywait_getchar(void)
{
	struct kbd_action
	{
		char key;
		int status;
	} action;

	int fd = open("/proc/kbd_bw", O_RDONLY);
	ioctl(fd, KBD_IOCTL_READKEY, &action);
	close(fd);

	return action.key;
}

void test_kbd_driver(char out1, int out2)
{
	struct kbd_action
	{
		char key;
		int status;
	} action;

	action.key = out1;
	action.status = out2;

	int fd = open("/proc/kbd_bw", O_RDONLY);
	ioctl(fd, KBD_IOCTL_TEST, &action);
	close(fd);	
}

