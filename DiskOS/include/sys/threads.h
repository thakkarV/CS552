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
} __attribute__((packed)) kthread_mutex_t;

void kthread_mutex_init(kthread_mutex_t *);
void kthread_mutex_destroy(kthread_mutex_t *);
void kthread_mutex_lock(kthread_mutex_t *);
void kthread_mutex_unlock(kthread_mutex_t *);

#endif // THREADS
