#include <sys/mutex.h>

// Atomically compare the current value of the mutex to @param old
// and set to @param new if equal, returning @ otherwise do not set
// and return 0.

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"
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
}
#pragma GCC diagnostic pop


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
