#include <sys/types.h>
#include <sys/sched.h>
#include <threads.h>
#include <kmalloc.h>

tid_t
thread_create(void * (* callable) (void *), void * arg)
{
	/* register this func to the scheduler to create a new thread */
	tid_t tid = sched_register_thread(callable, arg);
	return tid;
}
