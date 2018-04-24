#include <sysutils.h>
#include <sched.h>
#include <kvideo.h>
#include <timer.h>

void
msleep(uint32_t milliseconds)
{
	// first we convert the milliseconds
	// to the closest possible period of the system timer frequency
	// this timer frq is defined in the timer header where the PIT in configured
	size_t systicks =  (milliseconds * PIT_INT_FRQ) / 1000;
	__sleep_on(systicks);
}
