#include <sched.h>
#include <kmalloc.h>
#include <threads.h>
#include <types.h>

static volatile uint32_t __thread_count;

static task_struct_t * __idle_task;

static task_struct_t * __runq_head;
static task_struct_t * __current_task;

static task_struct_t * __waitq_head;

static task_struct_t * __doneq_head;


static task_struct_t * sched_select_next_rr(void);
static void do_idle(void);

/* default 8kB for each thread */
#define THREAD_STACK_SIZE 512
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
	"movl %1, %%esp\n\t"                                    \
	"pushl %2\n\t"                                          \
	"pushl %0\n\t"                                          \
	"sti\n\t"                                               \
	"ret\n\t"                                               \
	:                                                       \
	: "rm" (func), "rm" (esp), "rm" (exit_routine)          \
	: "memory")

/** IA-32 CALLING CONVENTION
 * Caller : push old EBP, move ESP to EBP, push all args, call
 * Callee : index args relative to ebp, save retval in EAX, leave, ret
**/
#define START_NEW_THREAD(func, esp, arg, ret, exit_routine)   \
	__asm__ volatile(                                         \
	"movl %0, %%esp\n\t"                                      \
	"pushl %1\n\t"                                            \
	"enter\n\t"                                               \
	"pushl %2\n\t"                                            \
	"sti\n\t"                                                 \
	"call %3\n\t"                                             \
	"movl %%eax, %4"                                          \
	"leave\n\t"                                               \
	"ret\n\t"                                                 \
	:                                                         \
	: "rm" (esp),                                             \
	  "rm" (exit_routine),                                    \
	  "rm" (arg),                                             \
	  "rm" (func),                                            \
	  "rm" (ret)                                              \
	: "memory")


void
init_sched(void)
{
	__idle_task = kmalloc(sizeof(task_struct_t));
	__idle_task->status = NEW;
	__idle_task->callable = do_idle;
	__idle_task->stack = kmalloc(sizeof(0x100));
	__idle_task->esp = __idle_task->stack + 0x100;
}


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
	ts->utime = 0;

	/* Setup thread environment */
	ts->callable = callable;
	// ts->args = args;
	ts->stack = kmalloc(THREAD_STACK_SIZE);
	ts->esp = ts->stack + THREAD_STACK_SIZE - 1;

	/* add task to run queue and init current task if this is the first */
	if (!__runq_head)
	{
		__runq_head = ts;
		__current_task = ts;
		ts->next = ts;
		ts->prev = ts;
	}
	else
		splice_inq(__runq_head, ts);

	return ts->tid;
}


void
sched_finalize_thread(void)
{
	__asm__ volatile ("cli"::);
	task_struct_t * curr_cpy = __current_task;
	__current_task = __current_task->prev;

	kfree(curr_cpy->stack);
	curr_cpy->status = EXITED;
	
	// remove from runq and add to waitq
	__runq_head = splice_outq(__runq_head, curr_cpy);
	
	if (__doneq_head)
		splice_inq(__doneq_head, curr_cpy);
	else
		__doneq_head = curr_cpy;
	
	// __current_task->retval = retval;
	__thread_count--;

	__asm__ volatile ("sti"::);
	while(1);
}


/* ROUND ROBIN POLICY BASED TASK PICKER */
static task_struct_t *
sched_select_next_rr(void)
{
	if (__current_task == __idle_task)
		__current_task = __runq_head;

	if (__waitq_head && __waitq_head->status == READY)
	{
		__current_task = __waitq_head;
		__waitq_head = NULL;
		return __current_task;
	}

	if (!__runq_head)
		return __idle_task;
	
	int i = 0;
	while (i++ != __thread_count + 1)
	{
		__current_task = __current_task->next;

		if (__current_task->status != EXITED && __current_task->status != BLOCKED)
			return __current_task;
	}

	return __idle_task;
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

	/* increment all sleep timers */
	if (__waitq_head && __waitq_head->utime >= __waitq_head->sleep_time)
	{
		// we would normally have a list for the queue so multiple tasks could sleep
		__waitq_head->status = READY;
	}

	/* pick next task to be run */
	if (__current_task->status != NEW)
		__current_task = sched_select_next_rr();


	/* nimble handinling required if the task has never run before */
	if(__current_task->status == NEW)
	{
		__current_task->status = RUNNING;
		START_NEW_THREAD_NOARG(
			__current_task->callable,
			__current_task->esp,
			sched_finalize_thread
		);
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
	task_struct_t * waitq_head = __waitq_head;
	if (__waitq_head)
	{
		do
		{
			++waitq_head->utime;
			waitq_head = waitq_head->next;
		} while (waitq_head != __waitq_head);
	}

	jiffies++;
	if (++__current_task->utime < 2)
		return;

	__current_task->utime = 0;
	schedule();
}


void
__sleep_on(uint32_t milliseconds)
{
	task_struct_t * curr_cpy = __current_task;
	__current_task = __current_task->prev;

	// set metadata
	curr_cpy->status = BLOCKED;
	curr_cpy->utime = 0;
	curr_cpy->sleep_time = milliseconds;

	// remove from runq and add to waitq
	__runq_head = splice_outq(__runq_head, curr_cpy);
	if (__waitq_head)
		splice_inq(__waitq_head, curr_cpy);
	else
		__waitq_head = curr_cpy;

	// block
	schedule();

	// when resumed, reset time counters
	__current_task->utime = 0;
	__current_task->sleep_time = 0;
}


static void
do_idle(void)
{
	while(1) __asm__ volatile("nop" ::);
}


void
splice_inq(task_struct_t * q_head, task_struct_t * element)
{
	element->next = q_head;
	element->prev = q_head->prev;

	q_head->prev->next = element;
	q_head->prev = element;
}

// always returns the new head pointer so that it is not leaked
task_struct_t *
splice_outq(task_struct_t * head, task_struct_t * element)
{
	task_struct_t * q_head;
	if (head == element)
		q_head = head;
	else
		q_head = head->next;

	element->prev->next = element->next;
	element->next->prev = element->prev;
	element->next = NULL;
	element->prev = NULL;

	return q_head;	
}