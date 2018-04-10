#ifndef THREADS
#define THREADS

#include <types.h>

tid_t thread_create(void (*) (void));
void thread_exit(void);

#endif // THREADS
