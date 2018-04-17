#ifndef SCHED
#define SCHED

#include <types.h>
#include <threads.h>

static long volatile jiffies;

typedef enum
{
	NEW = 0,
	READY,
	RUNNING,
	BLOCKED,
	EXITED
} TASK_STATUS;

typedef struct task_struct_t
{
	/* Context */
	uint32_t esp;

	/* Data */
	void * stack;
	void (*callable)(void);
	void * arg;
	void * retval;

	/* Metadata */
	tid_t tid;
	long utime;
	long sleep_time;
	long priority;
	TASK_STATUS status;
	struct task_struct_t * prev;
	struct task_struct_t * next;
} task_struct_t;


void schedule(void);
tid_t sched_register_thread(void (*) (void));
void sched_finalize_thread(void);
void init_sched(void);
void __sleep_on(uint32_t milliseconds);

void splice_inq(task_struct_t *, task_struct_t *);
task_struct_t * splice_outq(task_struct_t * head, task_struct_t * element);

#endif // SCHED
