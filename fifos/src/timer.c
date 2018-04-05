#include <timer.h>
#include <kportio.h>	/* inb, outb */
#include <kvideo.h>		/* printf */

void
init_pit(void)
{
	// TODO: check rate gen vs interrupt mode
	/* channel 0, lo/hi access, rate gen, 16-bit mode -- 00110100 */
	outb(PIT_COMMAND, 0x34);

	/* send lo byte and hi byte to set up desired frequency */
	outb(PIT_CHANNEL0_DATA, (PIT_BASE_FRQ / PIT_INT_FRQ) & 0xFF);
	outb(PIT_CHANNEL0_DATA, (PIT_BASE_FRQ / PIT_INT_FRQ) >> 8);
}
