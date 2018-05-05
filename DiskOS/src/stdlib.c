#include <sys/stdlib.h>
#include <sys/types.h>


void
memset(void * ptr, unsigned char val, size_t len)
{
	unsigned char * c = (unsigned char *) ptr;
	while(len--) *(c++) = val;
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
		while (len--) *(d++) = *(s++);
	}
	else
	{
		d += len - 1;
		s += len - 1;
		while (len--) *(d--) = *(s--);
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

	if (!str1 && !str2) return true;
	else return false;
}


bool
str_is_prefix(char * str, char * prefix)
{
	while (prefix && str)
	{
		if (*prefix++ != *str++)
			return false;
	}

	if (!prefix && str)
		return true;
	else
		return false;
}


size_t
strlen(char * str)
{
	size_t size = 0;
	while(*str++) size++;
	return size;
}


void
strcpy(char * source, char * dest)
{
	char c;
	while (c = *source++) *dest++ = c;
	*dest = NULL;
}


char *
strtok(char * str, char * delim)
{
	size_t delim_len = strlen(delim);
	int i;
	while (str)
	{
		for (i = 0; i < delim_len; i++)
		{
			if (*str == delim[i])
				return str;
		}
		str++;
	}

	return NULL;
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
