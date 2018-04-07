#ifndef SCHED
#define SCHED

#include <types.h>
#include <threads.h>

typedef enum
{
	NEW = 0,
	READY,
	RUNNING,
	BLOCKED,
	EXITED
} TASK_STATUS;


typedef struct task_struct
{
	tid_t tid;
	TCB * thread;
	enum TASK_STATUS;
	struct task_struct *prev;
	struct task_struct *next;
} task_struct;

static task_struct * __run_queue;
static task_struct * current;

void sched(void);
void dispatch(void);
void init_runq(void);
void register_thread(TCB *);
void remove_thread_byref(task_struct *);
void remove_thread_bytid(tid_t);

#endif // SCHED
