#include <sched.h>
#include <kmalloc.h>
#include <threads.h>
#include <types.h>

static uint32_t __thread_count;
static task_struct_t * __run_queue_head;
static task_struct_t * __current_task;

static task_struct_t * sched_select_next_rr(void);



/* default 8kB for each thread */
#define THREAD_STACK_SIZE 8192
#define MAX_THREADCOUNT 0x10

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
	"movl %0, %%esp\n\t"                                    \
	"pushl %1\n\t"                                          \
	"jmp %2\n\t"                                            \
	:                                                       \
	: "rm" (esp), "rm" (exit_routine), "rm" (func)          \
	: "memory")


tid_t
sched_register_thread(void (*callable) (void))
{
	__asm__ volatile ("cli" ::: "memory");

	if (__thread_count == MAX_THREADCOUNT)
	{
		__asm__ volatile ("sti" ::: "memory");
		return -1;
	}

	/* allocate task struct */
	task_struct_t * ts = kmalloc(sizeof(task_struct_t));

	/* init task metadata */
	ts->tid = __thread_count++;
	ts->status = NEW;
	ts->priority = 0;

	/* Setup thread environment */
	ts->callable = callable;
	// ts->args = args;
	ts->stack = kmalloc(THREAD_STACK_SIZE);
	ts->esp = ts->stack + THREAD_STACK_SIZE - 1;

	/* add task to run queue */
	if (!__run_queue_head)
	{
		__run_queue_head = ts;
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

	__asm__ volatile ("sti" ::: "memory");
	return ts->tid;
}


void
sched_finalize_thread(void)
{
	__asm__ volatile ("cli" ::: "memory");

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
	return __current_task->next;
}


void
schedule(void)
{
	__asm__ volatile ("cli" ::: "memory");

	/* Set current to ready, and save its machine context */
	if (__current_task->status != EXITED)
		__current_task->status = READY;
	
	SAVE_CONTEXT(__current_task->esp);
	/* FURTHER CODE MUST NOT MANIPULATE STACK IN NET EFFECT */

	/* Pick next task to be run */
	__current_task = sched_select_next_rr();

	/* Nimble handinling required if the task has never run before */
	if(__current_task->status == NEW)
	{
		__current_task->status = RUNNING;

		
		__asm__ volatile ("sti" ::: "memory");
		START_NEW_THREAD_NOARG(__current_task->callable, __current_task->esp, sched_finalize_thread);
	}
	else
	{
		/* Task switching is just infinite reucurison over all the running threads */
		__current_task->status = RUNNING;
		DISPATCH(__current_task->esp);

		__asm__ volatile ("sti" ::: "memory");
		return;
	}
}
