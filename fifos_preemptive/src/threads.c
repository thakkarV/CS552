#include <threads.h>
#include <sched.h>
#include <kmalloc.h>
#include <types.h>

tid_t
thread_create(void (* callable) (void))
{
	/* register this func to the scheduler to create a new thread */
	tid_t tid = sched_register_thread(callable);
	return tid;
}


void
thread_exit(void)
{
	sched_finalize_thread();
}
