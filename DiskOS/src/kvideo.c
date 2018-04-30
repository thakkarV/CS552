#include <kvideo.h>

/* Macros */
/* Some screen stuff. */
/* The number of columns. */
#define COLUMNS 80
/* The number of lines. */
#define LINES 25
/* The video memory address. */
#define VIDEO 0xB8000
/* The attribute of an character. */
static char ATTRIBUTE = 0x07;

/* Variables. */
/* Save the X position. */
static int xpos;
/* Save the Y position. */
static int ypos;
/* Point to the video memory. */
static volatile unsigned char *video;

static void __terminal_change_attrib(int code);


/* COLOR MAP */
#define DBLACK    0x0
#define DBLUE     0x1
#define DGREEN    0x2
#define DCYAN     0x3
#define DRED      0x4
#define DPINK     0x5
#define ORANGE    0x6
#define LGREY     0x7
#define DGREY     0x8
#define LBLUE     0x9
#define LGREEN    0xA
#define LCYAN     0xB
#define LRED      0xC
#define LPINK     0xD
#define YELLOW    0xE
#define WHITE     0xF


/* Clear the screen and initialize VIDEO, XPOS and YPOS. */
void
cls (void)
{
	int i;

	video = (unsigned char *) VIDEO;

	for (i = 0; i < COLUMNS * LINES * 2; i++)
		*(video + i) = 0;

	xpos = 0;
	ypos = 0;
}

/* Put the character C on the screen. */
void
putchar (int c)
{
	if (c == '\n' || c == '\r')
	{
		newline:
		while (xpos < COLUMNS)
		{
			*(video + (xpos + ypos * COLUMNS) * 2) = ' ' & 0xFF;
			xpos++;
		}

		xpos = 0;
		ypos++;
		if (ypos >= LINES)
			ypos = 0;
		
		return;
	}

	*(video + (xpos + ypos * COLUMNS) * 2) = c & 0xFF;
	*(video + (xpos + ypos * COLUMNS) * 2 + 1) = ATTRIBUTE;

	xpos++;
	if (xpos >= COLUMNS)
		goto newline;
}

/* Format a string and print it on the screen, just like the libc
function printf. */
void
printf (const char *format, ...)
{
	char **arg = (char **) &format;
	int c;
	char buf[20];

	arg++;

	while ((c = *format++) != 0)
	{
		if (c == '\033')
		{
			if ((c = *format++) == '[')
			{
				int code;
				while (c != 'm')
				{
					code = 0;
					c = *format++;
					while ((c != ';') && (c != 'm'))
					{
						code = (code << 3) + (code << 1) + (c - '0');
						c = *format++;
					}
					__terminal_change_attrib(code);
				}
			}
		}
		else if (c != '%')
		{
			putchar (c);
		}
		else
		{
			char *p, *p2;
			int pad0 = 0, pad = 0;

			c = *format++;
			if (c == '0')
			{
				pad0 = 1;
				c = *format++;
			}

			if (c >= '0' && c <= '9')
			{
				pad = c - '0';
				c = *format++;
			}

			switch (c)
			{
				case 'd':
				case 'u':
				case 'x':
				{
					itoa (buf, c, *((int *) arg++));
					p = buf;
					goto string;
				} break;
				case 's':
				{
					p = *arg++;
					if (! p)
						p = "(null)";

					string:
					for (p2 = p; *p2; p2++);
					for (; p2 < p + pad; p2++)
						putchar (pad0 ? '0' : ' ');
			
					while (*p)
						putchar (*p++);
				} break;
				default:
				{
					putchar (*((int *) arg++));
				} break;
			}
		}
	}
}



/* Convert the integer D to a string and save the string in BUF. If
BASE is equal to 'd', interpret that D is decimal, and if BASE is
equal to 'x', interpret that D is hexadecimal. */
void
itoa (char *buf, int base, int d)
{
	char *p = buf;
	char *p1, *p2;
	unsigned long ud = d;
	int divisor = 10;

	/* If %d is specified and D is minus, put `-' in the head. */
	if (base == 'd' && d < 0)
	{
		*p++ = '-';
		buf++;
		ud = -d;
	}
	else if (base == 'x')
		divisor = 16;

	/* Divide UD by DIVISOR until UD == 0. */
	do
	{
		int remainder = ud % divisor;

		*p++ = (remainder < 10) ? remainder + '0' : remainder + 'a' - 10;
	}
	while (ud /= divisor);

	/* Terminate BUF. */
	*p = 0;

	/* Reverse BUF. */
	p1 = buf;
	p2 = p - 1;

	while (p1 < p2)
	{
		char tmp = *p1;
		*p1 = *p2;
		*p2 = tmp;
		p1++;
		p2--;
	}
}


void
__terminal_change_attrib(int code)
{
	static const char COLOR_LUT[16] = 
	{
		DBLACK, DRED,  DGREEN, ORANGE,
		DBLUE,  DPINK, DCYAN,  LGREY,
		DGREY,  LRED,  LGREEN, YELLOW,
		LBLUE,  LPINK, LCYAN,  WHITE
	};

	/* background is the top 4 MSBs, foreground is the bottom 4 LSBs */

	// FG COLOR DARK
	if (code >= 30 && code <= 37)
	{
		ATTRIBUTE = (ATTRIBUTE & 0xF0) + COLOR_LUT[code - 30];
	}
	// BG COLOR DARK
	else if (code >= 40 && code <= 47)
	{
		ATTRIBUTE = (ATTRIBUTE & 0x0F) + (COLOR_LUT[code - 40] << 4);
	}
	// FG COLOR LIGHT
	else if (code >= 90 && code <= 97)
	{
		ATTRIBUTE = (ATTRIBUTE & 0xF0) + COLOR_LUT[code - 82];
	}
	// BG COLOR LIGHT
	else if (code >= 100 && code <= 107)
	{
		ATTRIBUTE = (ATTRIBUTE & 0x0F) + (COLOR_LUT[code - 92] << 4);
	}
}
