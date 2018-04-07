#ifndef THREADS
#define THREADS

typedef struct TCB
{
	void * (*callable)(void *);
	void * stack;
	uint32_t esp;
	void * retval;
} TCB;


void thread_create(void * (callable*) (void *));
void thread_exit(void);


#endif // THREADS
