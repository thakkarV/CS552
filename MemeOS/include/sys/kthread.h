#ifndef THREADS
#define THREADS

#include <sys/types.h>
// THREAD API
tid_t kthread_create(void * (*) (void *), void *);
void kthread_exit(void *);

#endif // THREADS
