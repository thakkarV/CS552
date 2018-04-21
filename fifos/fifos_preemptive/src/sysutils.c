#include <sysutils.h>
#include <sched.h>
#include <kvideo.h>

void
msleep(uint32_t milliseconds)
{
	__sleep_on(milliseconds);
}
