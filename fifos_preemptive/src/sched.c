#include <sched.h>
#include <kmalloc.h>
#include <threads.h>
#include <types.h>

static uint32_t __thread_count;
static task_struct_t * __run_queue_head;
static task_struct_t * __current_task;
static task_struct_t * __next_task;

static task_struct_t * sched_select_next_rr(void);


/* default 8kB for each thread */
#define THREAD_STACK_SIZE 512
#define MAX_THREADCOUNT 0x10

#define SAVE_CONTEXT_old(task_esp_addr) \
__asm__ volatile(                   \
	"movl %%esp, %0"                \
	: "=r" (task_esp_addr)          \
	:                               \
	: "memory")

#define SAVE_CONTEXT(task_esp_addr) \
__asm__ volatile(                   \
	"pushf\n\t"                     \
	"push %%eax\n\t"                \
	"push %%ebx\n\t"                \
	"push %%ecx\n\t"                \
	"push %%edx\n\t"                \
	"push %%esi\n\t"                \
	"push %%edi\n\t"                \
	"push %%ebp\n\t"                \
	"push %%ss\n\t"                 \
	"push %%ds\n\t"                 \
	"push %%fs\n\t"                 \
	"push %%gs\n\t"                 \
	"push %%es\n\t"                 \
	"movl %%esp, %0"                \
	: "=r" (task_esp_addr)          \
	:                               \
	: "memory")


#define DISPATCH(task_esp_addr) \
__asm__ volatile(               \
	"movl %0, %%esp\n\t"        \
	"pop %%es\n\t"              \
	"pop %%gs\n\t"              \
	"pop %%fs\n\t"              \
	"pop %%ds\n\t"              \
	"pop %%ss\n\t"              \
	"pop %%ebp\n\t"             \
	"pop %%edi\n\t"             \
	"pop %%esi\n\t"             \
	"pop %%edx\n\t"             \
	"pop %%ecx\n\t"             \
	"pop %%ebx\n\t"             \
	"pop %%eax\n\t"             \
	"popf\n\t"                  \
	:                           \
	: "r" (task_esp_addr)       \
	: "memory")


#define START_NEW_THREAD_NOARG(func, esp, exit_routine)     \
	__asm__ volatile(                                       \
	"movl %1, %%esp\n\t"                                    \
	"pushl %2\n\t"                                          \
	"pushl %0\n\t"                                          \
	"sti\n\t"                                             \
	"ret\n\t"                                             \
	:                                                       \
	: "rm" (func), "rm" (esp), "rm" (exit_routine)          \
	: "memory")


tid_t
sched_register_thread(void (*callable) (void))
{
	if (__thread_count == MAX_THREADCOUNT)
	{
		return -1;
	}

	/* allocate task struct */
	task_struct_t * ts = kmalloc(sizeof(task_struct_t));

	/* init task metadata */
	ts->tid = __thread_count++;
	ts->status = NEW;
	ts->priority = 0;
	ts->counter = 0;

	/* Setup thread environment */
	ts->callable = callable;
	// ts->args = args;
	ts->stack = kmalloc(THREAD_STACK_SIZE);
	ts->esp = ts->stack + THREAD_STACK_SIZE - 1;

	/* add task to run queue and init current task if this is the first */
	if (!__run_queue_head)
	{
		__run_queue_head = ts;
		__current_task = ts;
		ts->next = ts;
		ts->prev = ts;
	}
	else
	{
		ts->prev = __run_queue_head->prev;
		ts->next = __run_queue_head;

		__run_queue_head->prev->next = ts;
		__run_queue_head->prev = ts;
	}

	return ts->tid;
}


void
sched_finalize_thread(void)
{
	kfree(__current_task->stack);
	__current_task->status = EXITED;
	// __current_task->retval = retval;
	__thread_count--;

	schedule();
}


/* ROUND ROBIN POLICY BASED TASK PICKER */
static task_struct_t *
sched_select_next_rr(void)
{
	int i = 0;

	while (i++ != __thread_count+1)
	{
		if (__current_task->next->status != EXITED)
			return __current_task->next;
		else
			__current_task = __current_task->next;
	}

	printf("Halted!\n");
	while (true);
}


void
schedule(void)
{
	/* Set current to ready, and save its machine context */
	if (__current_task->status == RUNNING)
	{
		__current_task->status = READY;
		SAVE_CONTEXT(__current_task->esp);
	}
	/* FURTHER CODE MUST NOT MANIPULATE STACK IN NET EFFECT */

	/* pick next task to be run */
	if (__current_task->status != NEW)
		__current_task = sched_select_next_rr();

	/* nimble handinling required if the task has never run before */
	if(__current_task->status == NEW)
	{
		__current_task->status = RUNNING;
		START_NEW_THREAD_NOARG(__current_task->callable, __current_task->esp, sched_finalize_thread);
	}
	else
	{
		__current_task->status = RUNNING;
		/* task switching is just infinite reucurison over all the running threads */
		DISPATCH(__current_task->esp);
	}
}


void
do_timer(void)
{
	jiffies++;
	if (++__current_task->counter < 1000)
		return;

	__current_task->counter = 0;
	schedule();
}
