#ifndef SCHED
#define SCHED

#include <types.h>
#include <threads.h>


#define SAVE_CONTEXT(task_esp_ptr) \
__asm__ volatile( \
	"pushf\n\t" \
	"push %%eax\n\t" \
	"push %%ebx\n\t" \
	"push %%ecx\n\t" \
	"push %%edx\n\t" \
	"push %%ss\n\t" \
	"push %%ds\n\t" \
	"push %%fs\n\t" \
	"push %%gs\n\t" \
	"push %%es\n\t" \
	"movl %%esp, (%0)" \
	: \
	: "r" (task_esp_ptr) \
)


#define DISPATCH(task_esp_ptr) \
__asm__ volatile( \
	"movl (%0), %esp" \
	"pop %%es\n\t" \
	"pop %%gs\n\t" \
	"pop %%fs\n\t" \
	"pop %%ds\n\t" \
	"pop %%ss\n\t" \
	"pop %%edx\n\t" \
	"pop %%ecx\n\t" \
	"pop %%ebx\n\t" \
	"pop %%eax\n\t" \
	"popf\n\t" \
	: \
	: "r" (task_esp_ptr) \
)

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
	TCB * tcb;
	long counter;
	long priority;
	TASK_STATUS status;
	struct task_struct * prev;
	struct task_struct * next;
} task_struct;


void schedule(void);
void dispatch(void);
void init_runq(void);
void sched_register_tcb(TCB *);
void remove_thread_byref(task_struct *);
void remove_thread_bytid(tid_t);

#endif // SCHED
