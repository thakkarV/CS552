#ifndef KSTDLIB
#define KSTDLIB

#include <sys/types.h>

// MEM
void memset(void * ptr, unsigned char val, size_t len);
void memcpy(void * dest, void * source, size_t len);

// STR
bool strcmp(char *, char *);

// PORT IO
uint8_t inb(uint16_t port);
void outb(uint16_t port, uint8_t val);

#endif // KSTDLIB
