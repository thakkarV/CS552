#ifndef KSTDLIB
#define KSTDLIB

#include <sys/types.h>

// MEM
void memset(void * ptr, unsigned char val, size_t len);
void memcpy(void * dest, void * source, size_t len);

// STR
bool strcmp(char *, char *);
bool str_is_prefix(char *, char *);
size_t strlen(char *);
void strcpy(char *, char *);
char * strtok(char *, char *);
void sprintf (char * dest, const char *format,...);
void itoa (char *buf, int base, int d);

// PORT IO
uint8_t inb(uint16_t port);
void outb(uint16_t port, uint8_t val);

#endif // KSTDLIB
