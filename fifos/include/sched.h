#ifndef SCHED
#define SCHED

#define tid_t unsigned int

typedef struct runq
{
	int (*task)();
	tid_t tid; // thread ID
	int is_ready;
	struct runq *prev;
	struct runq *next;
} runq;

void sched(void);
void init_runq(void);
void remove_thread_byref(runq *);
void remove_thread_bytid(tid_t);


// stackless threads
int thread1(void);
int thread2(void);

#endif // SCHED