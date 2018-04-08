#ifndef SCHED
#define SCHED

#include <types.h>
#include <threads.h>

#define MAX_THREADCOUNT 0x10

/* default 8kB for each thread */
#define THREAD_STACK_SIZE 8192


#define SAVE_CONTEXT(task_esp_addr) \
__asm__ volatile( \
	"pushf\n\t" \
	"push %%eax\n\t" \
	"push %%ebx\n\t" \
	"push %%ecx\n\t" \
	"push %%edx\n\t" \
	"push %%esi\n\t" \
	"push %%edi\n\t" \
	"push %%ss\n\t" \
	"push %%ds\n\t" \
	"push %%fs\n\t" \
	"push %%gs\n\t" \
	"push %%es\n\t" \
	"movl %%esp, %0" \
	: "=r" (task_esp_addr) \
	: \
	: "memory" \
)


#define DISPATCH(task_esp_addr) \
__asm__ volatile( \
	"movl %0, %%esp\n\t" \
	"pop %%es\n\t" \
	"pop %%gs\n\t" \
	"pop %%fs\n\t" \
	"pop %%ds\n\t" \
	"pop %%ss\n\t" \
	"pop %%edi\n\t" \
	"pop %%esi\n\t" \
	"pop %%edx\n\t" \
	"pop %%ecx\n\t" \
	"pop %%ebx\n\t" \
	"pop %%eax\n\t" \
	"popf\n\t" \
	: \
	: "r" (task_esp_addr) \
	: "memory" \
)


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
	void * (*callable)(void *);
	void * args;
	void * retval;

	/* Metadata */
	tid_t tid;
	long counter;
	long priority;
	TASK_STATUS status;
	struct task_struct_t * prev;
	struct task_struct_t * next;
} task_struct_t;


void schedule(void);
tid_t sched_register_thread(void * (*) (void *), void *);
void sched_finalize_thread(void);

#endif // SCHED
