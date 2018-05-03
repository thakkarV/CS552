#ifndef SYS_SCHED
#define SYS_SCHED

#include <sys/types.h>
#include <sys/threads.h>
#include <sys/vfs.h>

static long volatile jiffies;

typedef enum TASK_STATUS
{
	NEW = 0,
	READY,
	RUNNING,
	BLOCKED,
	EXITED
} TASK_STATUS;

typedef struct task_struct_t
{
	/* CONTEXT */
	uint32_t esp;

	/* DATA */
	void * stack;
	void * (*callable)(void *);
	void * arg;
	void * retval;

	/* METADATA */
	tid_t tid;
	long utime;
	long sleep_time;
	long priority;
	TASK_STATUS status;

	/* FILESYSTEM */
    FILE * fd_table[NUM_MAX_FD];

	/* SCHED Q PTR */
	struct task_struct_t *prev;
	struct task_struct_t *next;
} __attribute__((packed)) task_struct_t;


void schedule(void);
tid_t sched_register_thread(void * (*) (void *), void *);
void sched_finalize_thread(void);
void init_sched(void);
void __sleep_on(uint32_t milliseconds);

#endif // SCHED
