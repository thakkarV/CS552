#include "stdlib.h"

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

	while (len--)
		*(d++) = *(s++);
}