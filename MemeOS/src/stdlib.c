#include <sys/stdlib.h>
#include <sys/types.h>


void memset(void *ptr, unsigned char val, size_t len)
{
	unsigned char *c = (unsigned char *)ptr;
	while (len--)
		*(c++) = val;
}


void memcpy(void *dest, void *source, size_t len)
{
	unsigned char *d = (unsigned char *)dest;
	unsigned char *s = (unsigned char *)source;

	if (source == dest)
		return;

	// take care of overlapping region copes
	if (source > dest) {
		while (len--)
			*(d++) = *(s++);
	} else {
		d += len - 1;
		s += len - 1;
		while (len--)
			*(d--) = *(s--);
	}
}


bool strcmp(char *str1, char *str2)
{
	while (*str1 && *str2) {
		if (*str1++ != *str2++)
			return false;
	}
	return (!*str1 && !*str2);
}


/*
 * @param str: input string to be matched
 * @param prefix: input string that is to be matched as a prefix of @str
 * @return true if the @prefix is a prefix of @str
 **/
bool str_is_prefix(char *str, char *prefix)
{
	while (*prefix && *str) {
		if (*prefix++ != *str++)
			return false;
	}

	if (!*prefix)
		return true;
	else
		return false;
}


size_t strlen(char *str)
{
	size_t size = 0;
	while (*str++)
		size++;
	return size;
}


void strcpy(char *source, char *dest)
{
	char c;
	while ((c = *source++))
		*dest++ = c;
	*dest = NULL;
}


char *strtok(char *str, char *delim)
{
	size_t delim_len = strlen(delim);
	int i;
	while (*str) {
		for (i = 0; i < delim_len; i++) {
			if (*str == delim[i])
				return str;
		}
		str++;
	}

	return NULL;
}


void sprintf(char *dest, const char *format, ...)
{
	char **arg = (char **)&format;
	int c;
	char buf[20];

	arg++;
	while ((c = *format++) != 0) {
		if (c != '%') {
			*dest++ = c;
		} else {
			char *p, *p2;
			int pad0 = 0, pad = 0;

			c = *format++;
			if (c == '0') {
				pad0 = 1;
				c	= *format++;
			}

			if (c >= '0' && c <= '9') {
				pad = c - '0';
				c   = *format++;
			}

			switch (c) {
				case 'd':
				case 'u':
				case 'x': {
					itoa(buf, c, *((int *)arg++));
					p = buf;
					goto string;
				} break;
				case 's': {
					p = *arg++;
					if (!p)
						p = "(null)";

				string:
					for (p2 = p; *p2; p2++)
						;
					for (; p2 < p + pad; p2++)
						*dest++ = pad0 ? '0' : ' ';

					while (*p)
						*dest++ = *p++;
				} break;
				default: {
					*dest++ = *((int *)arg++);
				} break;
			}
		}
	}
}


/* Convert the integer D to a string and save the string in BUF. If
BASE is equal to 'd', interpret that D is decimal, and if BASE is
equal to 'x', interpret that D is hexadecimal. */
void itoa(char *buf, int base, int d)
{
	char *p = buf;
	char *p1, *p2;
	unsigned long ud = d;
	int divisor		 = 10;

	/* If %d is specified and D is minus, put `-' in the head. */
	if (base == 'd' && d < 0) {
		*p++ = '-';
		buf++;
		ud = -d;
	} else if (base == 'x')
		divisor = 16;

	/* Divide UD by DIVISOR until UD == 0. */
	do {
		int remainder = ud % divisor;

		*p++ = (remainder < 10) ? remainder + '0' : remainder + 'a' - 10;
	} while (ud /= divisor);

	/* Terminate BUF. */
	*p = 0;

	/* Reverse BUF. */
	p1 = buf;
	p2 = p - 1;

	while (p1 < p2) {
		char tmp = *p1;
		*p1		 = *p2;
		*p2		 = tmp;
		p1++;
		p2--;
	}
}

uint8_t inb(uint16_t port)
{
	uint8_t ret;
	__asm__ volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
	return ret;
}


void outb(uint16_t port, uint8_t val)
{
	__asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}
