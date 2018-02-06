#include <sys/ioctl.h>
#include <fcntl.h>

#define KBD_IOCTL_READ_KEY _IOR(0, 7, char key)

char my_getchar(void)
{
	char c;
	static char scancode[128] = "\0\e1234567890-=\177\tqwertyuiop[]\n\0asdfghjkl;'`\0\\zxcvbnm,./\0*\0 \0\0\0\0\0\0\0\0\0\0\0\0\000789-456+1230.\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";

	/* Poll keyboard status register at port 0x64 checking bit 0 to see if
	* output buffer is full. We continue to poll if the msb of port 0x60
	* (data port) is set, as this indicates out-of-band data or a release
	* keystroke
	*/
	while( !(inb( 0x64 ) & 0x1) || ( ( c = inb( 0x60 ) ) & 0x80 ) );

	return scancode[(int)c];
}

static inline unsigned char inb(unsigned short usPort)
{

	unsigned char uch;
	asm volatile( "inb %1,%0" : "=a" (uch) : "Nd" (usPort) );
	return uch;
}

static inline void outb(unsigned char uch, unsigned short usPort)
{

	asm volatile( "outb %0,%1" : : "a" (uch), "Nd" (usPort) );
}