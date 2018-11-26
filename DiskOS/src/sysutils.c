#include <sys/timer.h>
#include <sched.h>

void
msleep(unsigned long milliseconds)
{
	// first we convert the milliseconds
	// to the closest possible period of the system timer frequency
	// this timer frq is defined in the timer header where the PIT in configured
	__sleep_on((milliseconds * PIT_INT_FRQ) / 1000);
}
