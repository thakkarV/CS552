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
	// 1 = unlocked, 0 = locked
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
	while (!__sync_bool_cmpxchg(mutex, 1, 0))
	{
		__asm__ volatile("pause");
		// schedule();
	}
}


void
kthread_mutex_unlock(volatile kthread_mutex_t *mutex)
{
	mutex->flag = 1;
}


// Atomically compare the current value of the mutex to @param old
// and set to @param new if equal, returning @ otherwise do not set
// and return 0.
static bool
__sync_bool_cmpxchg(volatile kthread_mutex_t *mutex, int old, int new)
{
	__asm__ volatile
	(
		"LOCK cmpxchg %[source], %[dest]\n\t"
		"setz %%al\n\t"
		:
		: [source] "d" (new), [accum] "a" (old), [dest] "m" (mutex->flag)
		: "memory"
	);

	return;
}
