#include <sched.h>
#include <kmalloc.h>
#include <threads.h>
#include <types.h>

static volatile uint32_t __thread_count;
static volatile uint32_t __tid_counter;

// sched tasks
static task_struct_t *__idle_task;
static task_struct_t *__current_task;

// Sched queues
static task_struct_t *__runq_head;
static task_struct_t *__waitq_head;
static task_struct_t *__doneq_head;

// sched ops
static void do_idle(void);
static task_struct_t *sched_select_next_rr(void);
static void service_waitq(void);

// list ops
static void splice_inq(task_struct_t **, task_struct_t *);
static void splice_outq(task_struct_t **, task_struct_t *);

/* default 8kB for each thread */
#define THREAD_STACK_SIZE 8196
#define MAX_THREADCOUNT 0x10

#define SAVE_CONTEXT(task_esp_addr) \
__asm__ volatile(         \
	"pushf\n\t"           \
	"push %%eax\n\t"      \
	"push %%ebx\n\t"      \
	"push %%ecx\n\t"      \
	"push %%edx\n\t"      \
	"push %%esi\n\t"      \
	"push %%edi\n\t"      \
	"push %%ebp\n\t"      \
	"push %%ss\n\t"       \
	"push %%ds\n\t"       \
	"push %%fs\n\t"       \
	"push %%gs\n\t"       \
	"push %%es\n\t"       \
	"movl %%esp, %0"      \
	: "=r"(task_esp_addr) \
	:                     \
	: "memory")

#define DISPATCH(task_esp_addr) \
__asm__ volatile(        \
	"movl %0, %%esp\n\t" \
	"pop %%es\n\t"       \
	"pop %%gs\n\t"       \
	"pop %%fs\n\t"       \
	"pop %%ds\n\t"       \
	"pop %%ss\n\t"       \
	"pop %%ebp\n\t"      \
	"pop %%edi\n\t"      \
	"pop %%esi\n\t"      \
	"pop %%edx\n\t"      \
	"pop %%ecx\n\t"      \
	"pop %%ebx\n\t"      \
	"pop %%eax\n\t"      \
	"popf\n\t"           \
	:                    \
	: "r"(task_esp_addr) \
	: "memory")

#define START_NEW_THREAD_NOARG(func, esp, exit_routine) \
	__asm__ volatile(                                   \
		"movl %1, %%esp\n\t"                            \
		"pushl %2\n\t"                                  \
		"pushl %0\n\t"                                  \
		"sti\n\t"                                       \
		"ret\n\t"                                       \
		:                                               \
		: "rm"(func), "rm"(esp), "rm"(exit_routine)     \
		: "memory")

/** IA-32 CALLING CONVENTION
 * Caller : push old EBP, move ESP to EBP, push all args, call
 * Callee : index args relative to ebp, save retval in EAX, leave, ret
**/
#define START_NEW_THREAD(func, esp, arg, ret_ptr, exit_routine) \
	__asm__ volatile(                                       \
		"movl %1, %%esp\n\t"                                \
		"pushl %2\n\t"                                      \
		"movl %%esp, %%ebp\n\t"                             \
		"pushl %3\n\t"                                      \
		"sti\n\t"                                           \
		"call %4\n\t"                                       \
		"movl %%eax, %0\n\t"                                \
		"movl %%ebp, %%esp\n\t"                             \
		"ret\n\t"                                           \
		: "=r"(ret_ptr)                                     \
		: "rm"(esp),                                        \
		  "rm"(exit_routine),                               \
		  "rm"(arg),                                        \
		  "rm"(func)                                        \
		: "memory")


void
init_sched(void)
{
	__idle_task = kmalloc(sizeof(task_struct_t));
	__idle_task->status = NEW;
	__idle_task->callable = do_idle;
	__idle_task->stack = kmalloc(0x100);
	__idle_task->esp = __idle_task->stack + 0x100 - 1;
}


tid_t
sched_register_thread(void * (*callable) (void *), void * arg)
{
	if (__thread_count == MAX_THREADCOUNT)
		return -1;

	/* allocate task struct */
	task_struct_t * ts = kmalloc(sizeof(task_struct_t));

	/* init task metadata */
	ts->tid = __tid_counter++;
	ts->status = NEW;
	ts->priority = 0;
	ts->utime = 0;

	/* Setup thread environment */
	ts->callable = callable;
	ts->arg = arg;
	ts->stack = kmalloc(THREAD_STACK_SIZE);
	ts->esp = ts->stack + THREAD_STACK_SIZE - 1;

	/* add task to run queue and init current task if this is the first */
	if (!__current_task)
		__current_task = ts;
	
	splice_inq(&__runq_head, ts);
	__thread_count++;
	return ts->tid;
}


void
sched_finalize_thread(void)
{
	__asm__ volatile("cli":::"memory");
	task_struct_t * curr_cpy = __current_task;
	__current_task = __current_task->prev;

	kfree(curr_cpy->stack);
	curr_cpy->status = EXITED;

	// remove from runq and add to waitq
	splice_outq(&__runq_head, curr_cpy);
	splice_inq(&__doneq_head, curr_cpy);

	// __current_task->retval = retval;
	__thread_count--;
	__asm__ volatile("sti":::"memory");

	// this schedule will run in the freeing thread's stack space
	// but this is not an issue since this exit routine will not be interrupted
	// and the stack will be marked free and therefore reaped
	schedule();
}


/* ROUND ROBIN POLICY BASED TASK PICKER */
static task_struct_t *
sched_select_next_rr(void)
{
	if (__current_task == __idle_task)
		__current_task = __runq_head;

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
	/* PART 0: Set current to ready, and save its machine context */
	if (__current_task->status == RUNNING)
	{
		__current_task->status = READY;
		SAVE_CONTEXT(__current_task->esp);
	}

	/* FURTHER CODE MUST NOT MANIPULATE STACK IN NET EFFECT */

	/* PART 1: check all sleeping tasks to see if they are ready to be woken up */
	if (__waitq_head)
		service_waitq();

	/* PART 2: pick next task to be run */
	if (__current_task->status != NEW)
		__current_task = sched_select_next_rr();

	/* PART 3: switch threads */
	if (__current_task->status == NEW)
	{
		/* nimble handinling required if the task has never run before */
		__current_task->status = RUNNING;
		START_NEW_THREAD(
			__current_task->callable,
			__current_task->esp,
			__current_task->arg,
			__current_task->retval,
			sched_finalize_thread
		);
	}
	else
	{
		__current_task->status = RUNNING;
		/* task switching is just infinite recurison over all the running threads */
		DISPATCH(__current_task->esp);
	}
}


void
do_timer(void)
{
	task_struct_t *waitq_head = __waitq_head;
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


static void
service_waitq(void)
{
	task_struct_t *waitq = __waitq_head;
	task_struct_t *tmp;

	// TODO: Verify that this loop works and looks at each task once
	do
	{
		if (waitq->utime >= waitq->sleep_time)
		{
			tmp = waitq->next;
			waitq->status = READY;
			splice_outq(&__waitq_head, waitq);
			splice_inq(&__runq_head, waitq);
			waitq = tmp;
		}
		else
		{
			waitq = waitq->next;
		}
	// keep checking until either we loop back to the queue head
	// OR the queue has been emptied
	} while (__waitq_head && waitq != __waitq_head);
}


void
__sleep_on(uint32_t milliseconds)
{
	__asm__ volatile("cli":::"memory");

	task_struct_t *curr_cpy = __current_task;
	__current_task = __current_task->prev;

	// set metadata
	curr_cpy->status = BLOCKED;
	curr_cpy->utime = 0;
	curr_cpy->sleep_time = milliseconds;

	// remove from runq and add to waitq
	splice_outq(&__runq_head, curr_cpy);
	splice_inq(&__waitq_head, curr_cpy);

	/* TODO: make this better unified into the scheduler if possible
	 * this is somewhat hacky
	 * ideally save context should only be called by the scheduler
	 * but since we are setting the current task to blocked
	 * and then changing the pointer "__current_task" we have to do it here
	 * otherwise we will not be able to set the context for this thread
	 * and all hell will break loose when we try to resume it
	**/
	SAVE_CONTEXT(curr_cpy->esp);

	// block
	__asm__ volatile("sti":::"memory");
	schedule();
	
	// when resumed, reset time counters
	__current_task->utime = 0;
	__current_task->sleep_time = 0;
}


static void
do_idle(void)
{
	while (true)
		__asm__ volatile("nop":::"memory");
}


static void
splice_inq(task_struct_t **head_ptr, task_struct_t *element)
{
	if (*head_ptr)
	{
		element->next = *head_ptr;
		element->prev = (*head_ptr)->prev;

		(*head_ptr)->prev->next = element;
		(*head_ptr)->prev = element;
	}
	else
	{
		*head_ptr = element;
		(*head_ptr)->next = element;
		(*head_ptr)->prev = element;
	}
}


static void
splice_outq(task_struct_t **head_ptr, task_struct_t *element)
{
	// TODO: make sure this makes sense
	if (!(*head_ptr))
		return;

	// element being splced out is the head
	if (element == *head_ptr)
	{
		// only one element left in queue
		if (*head_ptr == (*head_ptr)->next)
		{
			*head_ptr = NULL;
			return;
		}
		// more than one left in queue
		else
		{
			*head_ptr = element->next;
		}
	}

	element->prev->next = element->next;
	element->next->prev = element->prev;
	element->next = NULL;
	element->prev = NULL;
}