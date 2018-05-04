#include <sys/threads.h>
#include <sys/types.h>
#include <sys/sched.h>
#include <kmalloc.h>

static bool __sync_bool_cmpxchg(volatile kthread_mutex_t *, int, int);

tid_t
kthread_create(void * (* callable) (void *), void * arg)
{
	/* register this func to the scheduler to create a new thread */
	tid_t tid = sched_register_thread(callable, arg);
	return tid;
}


void
kthread_mutex_init(volatile kthread_mutex_t *mutex)
{
	// 1 = unloced, 0 = locked
	mutex->flag = 1;
}


void
kthread_mutex_destroy(volatile kthread_mutex_t *mutex)
{
	return;
}


void
kthread_mutex_lock(volatile kthread_mutex_t *mutex)
{
	while (!__sync_bool_cmpxchg(mutex, 1, 0));
		// schedule();
}


void
kthread_mutex_unlock(volatile kthread_mutex_t *mutex)
{
	mutex->flag = 1;
}


static bool
__sync_bool_cmpxchg(volatile kthread_mutex_t *mutex, int old, int new)
{
	__asm__ volatile
	(
		"movl %[old], %%eax\n\t"
		"lock cmpxchgl %[new], %[flag]\n\t"
		"sete %%al\n\t"
		: 
		: [new] "d" (new), [old] "a" (old), [flag] "m" (mutex->flag)
		: "memory"
	);

	return;
}