#include <stdlib.h>
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
