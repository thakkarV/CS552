#include <sys/stdlib.h>
#include <sys/types.h>


void
memset(void * ptr, unsigned char val, size_t len)
{
	unsigned char * c = (unsigned char *) ptr;

	while(len--)
		*(c++) = val;
}


void
memcpy(void * dest, void * source, size_t len)
{
	unsigned char *d = (unsigned char *) dest;
	unsigned char *s = (unsigned char *) source;

	if (source == dest)
		return;

	// take care of overlapping region copes
	if (source > dest)
	{
		while (len--)
			*(d++) = *(s++);
	}
	else
	{
		d += len - 1;
		s += len - 1;
		while (len--)
			*(d--) = *(s--);
	}
}

bool
strcmp(char *str1, char *str2)
{
	while (str1 && str2)
	{
		if (str1++ != str2++)
			return false;
	}

	if (!str1 && !str2)
		return true;
	else
		return false;
}

uint8_t
inb(uint16_t port)
{
	uint8_t ret;
	__asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
	return ret;
}


void
outb(uint16_t port, uint8_t val)
{
	__asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}
