#include "multiboot.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#if !defined(__i386__)
#error "Compile with ix86-elf compiler."
#endif

/* Hardware text mode color constants. */
enum vga_color {
	VGA_COLOR_BLACK = 0,
	VGA_COLOR_BLUE = 1,
	VGA_COLOR_GREEN = 2,
	VGA_COLOR_CYAN = 3,
	VGA_COLOR_RED = 4,
	VGA_COLOR_MAGENTA = 5,
	VGA_COLOR_BROWN = 6,
	VGA_COLOR_LIGHT_GREY = 7,
	VGA_COLOR_DARK_GREY = 8,
	VGA_COLOR_LIGHT_BLUE = 9,
	VGA_COLOR_LIGHT_GREEN = 10,
	VGA_COLOR_LIGHT_CYAN = 11,
	VGA_COLOR_LIGHT_RED = 12,
	VGA_COLOR_LIGHT_MAGENTA = 13,
	VGA_COLOR_LIGHT_BROWN = 14,
	VGA_COLOR_WHITE = 15,
};


#define FBUF_START 0xB8000;
#define COLS 80
#define ROWS 25

static size_t xpos;
static size_t ypos;
volatile uint16_t *fbuf;
static uint8_t color;

void cmain(void);
void init(void);
void cls(void);
void print_str(char *);
size_t strlen(char *);
void putchar(char);
static inline uint8_t vga_entry_color(enum vga_color fg, enum vga_color bg);
static inline uint16_t vga_entry(unsigned char uc, uint8_t color);


void
cmain(void)
{
	init();
	cls();
	return;
}


void
print_str(char * str)
{
	size_t len = strlen(str);
	
	size_t i;
	for (i = 0; i < len; i++)
	{
		putchar(str[i]);
	}
}


void
putchar(char c)
{
	size_t index;
	if (c == '\n' || c == '\r')
	{
		newline:
			xpos = 0;
			ypos++;
			if (ypos >= COLS)
		 		ypos = 0;
			return;
	}
     
	index = (ypos * COLS) + xpos;
	fbuf[index] = vga_entry(c, color);

	xpos++;
	if (xpos >= COLS)
		goto newline;
}


size_t
strlen(char * str)
{
	size_t len = 0;
	while (str[len++]);
	return len;
}


void
cls(void)
{
	int x;
	int y;
	size_t index;

	for (x = 0; x < COLS; x ++)
	{
		for (y = 0; y < ROWS; y++)
		{
			index = (y * COLS) + x;
			fbuf[index] = (' ', color);
		}
	}
}


void
init(void)
{
	fbuf = FBUF_START;
	xpos = 0;
	ypos = 0;
	color = vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
}


static inline uint8_t vga_entry_color(enum vga_color fg, enum vga_color bg)
{
	return fg | bg << 4;
}

static inline uint16_t vga_entry(unsigned char uc, uint8_t color)
{
	return (uint16_t) uc | (uint16_t) color << 8;
}
