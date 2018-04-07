#ifndef THREADS
#define THREADS

#include <types.h>

#define MAX_THREADCOUNT 0x10

/* default 8kB for each thread */
#define THREAD_STACK_SIZE 8192

typedef struct TCB
{
	uint32_t esp;
	void * stack;
	void * (*callable)(void *);
	void * retval;
} TCB;

int thread_create(void * (*callable) (void *));
void thread_exit(void);


#endif // THREADS
