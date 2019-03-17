#ifndef THREADS
#define THREADS

#include <sys/types.h>
// THREAD API
tid_t kthread_create(void * (*) (void *), void *);
void kthread_exit(void *);

// MUTEX API
typedef struct mutex
{
    int flag;
    int _padding[15];
    // assume that the cache line size is 64 bytes and align accordingly
} __attribute__((aligned(0x40))) kthread_mutex_t;

void kthread_mutex_init(volatile kthread_mutex_t *);
void kthread_mutex_destroy(volatile kthread_mutex_t *);
void kthread_mutex_lock(volatile kthread_mutex_t *);
void kthread_mutex_unlock(volatile kthread_mutex_t *);

#endif // THREADS
