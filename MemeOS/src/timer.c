#include <kvideo.h> /* printf */
#include <sys/stdlib.h> /* inb, outb */
#include <sys/timer.h>

void init_pit(void)
{
	/* channel 0, lo/hi access, rate generator, 16-bit mode -- 00110110 */
	outb(PIT_COMMAND, 0x34);

	/* send lo byte and hi byte to set up desired frequency */
	outb(PIT_CHANNEL0_DATA, (PIT_BASE_FRQ / PIT_INT_FRQ) & 0xFF);
	outb(PIT_CHANNEL0_DATA, (PIT_BASE_FRQ / PIT_INT_FRQ) >> 8);
}
