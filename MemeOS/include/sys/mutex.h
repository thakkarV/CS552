#ifndef MUTEX_H
#define MUTEX_H

#include <sys/types.h>

typedef struct
{
    int flag;
    int _padding[15];
    // assume that the cache line size is 64 bytes and align accordingly
} __attribute__((aligned(0x40))) kthread_mutex_t;

void kthread_mutex_init(volatile kthread_mutex_t *);
void kthread_mutex_destroy(volatile kthread_mutex_t *);
void kthread_mutex_lock(volatile kthread_mutex_t *);
void kthread_mutex_unlock(volatile kthread_mutex_t *);

#endif // MUTEX_H
