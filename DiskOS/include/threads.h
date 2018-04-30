#ifndef THREADS
#define THREADS

#include <sys/types.h>

tid_t thread_create(void * (*) (void *), void *);
void thread_exit(void *);

#endif // THREADS
