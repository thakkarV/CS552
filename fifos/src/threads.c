#include <threads.h>
#include <sched.h>
#include <kmalloc.h>
#include <types.h>

tid_t
thread_create(void * (* callable) (void *), void * args)
{
	__asm__ volatile("cli"::);

	/* register this func to the scheduler to create a new thread */
	tid_t tid = sched_register_thread(callable, args);

	__asm__ volatile("sti"::);
	return tid;
}


void
thread_exit(void)
{
	__asm__ volatile("cli"::);

	sched_finalize_thread();

	__asm__ volatile("sti"::);
}
