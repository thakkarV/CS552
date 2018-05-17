#include <sys/sched.h>
#include <sys/types.h>
#include <sys/stdlib.h>
#include <kmalloc.h>

static volatile uint32_t __thread_count;
static volatile uint32_t __tid_counter;

// sched tasks
static task_struct_t *__idle_task;
task_struct_t *__current_task;

// Sched queues
static task_struct_t *__runq_head;
static task_struct_t *__waitq_head;
static task_struct_t *__doneq_head;

// sched ops
static void * do_idle(void *);
static task_struct_t *sched_select_next_rr(void);
static void service_waitq(void);

// list ops
static void splice_inq(task_struct_t **, task_struct_t *);
static void splice_outq(task_struct_t **, task_struct_t *);

#define THREAD_DUMMY_CONTEXT_START

/* default 8kB for each thread */
#define THREAD_STACK_SIZE 0x2000
#define MAX_THREADCOUNT 0x10

/* Context: Fake Generator
 ! HARD TO UNDERSTAND
 * Sets up a fake context for a new thread to be started
 * 1) save the ebx since it will be used as scratch register to setup dummy
 * 2) swap out esp to swtich to the new stack
 * 3) push arguemnt, finalizer func and thread func
 * 4) push base pointer for this new stack using ebx as scratch
 * 5.0) push two 4 byte zeros. 
 * 5.1) second zero for ebx pop inserted by gcc
 * 5.2) first zero for dummy ebp for the sched() epilogue
 * 6) push 0x200 -> flags register (IF - interrupt enable)
 * 7) push seven zeros for all 32 bit registers
 * 8) push segment registers
 * 9) swap back esp to the stack of thread registrar
 * A) pop ebp that was used as a scratch register
**/
#define SETUP_NEW_THREAD(new, finalizer)\
	__asm__ volatile                    \
	(                                   \
		"pushl %%ebx\n\t"               \
		"xchgl %%esp, %[esp]\n\t"       \
		"pushl %[arg]\n\t"              \
		"pushl %[finlizer]\n\t"         \
		"pushl %[func]\n\t"             \
		"movl  %%esp, %%ebx\n\t"        \
		"addl  $0x4, %%ebx\n\t"         \
		"pushl %%ebx\n\t"               \
		"pushl $0\n\t"                  \
		"pushl $0\n\t"                  \
		"pushl $0x200\n\t"              \
		"pushl $0\n\t"                  \
		"pushl $0\n\t"                  \
		"pushl $0\n\t"                  \
		"pushl $0\n\t"                  \
		"pushl $0\n\t"                  \
		"pushl $0\n\t"                  \
		"pushl $0\n\t"                  \
		"push  %%ss\n\t"                \
		"push  %%ds\n\t"                \
		"push  %%es\n\t"                \
		"push  %%fs\n\t"                \
		"push  %%gs\n\t"                \
		"xchgl %%esp, %[esp]\n\t"       \
		"popl  %%ebx\n\t"               \
		:                               \
		: [arg]      "m"(new->arg),     \
		  [esp]      "m"(new->esp),     \
		  [finlizer] "r"(finalizer),    \
		  [func]     "m"(new->callable) \
		: "memory"                      \
	)


/* Context: Save
 * saves the running context onto the current stack
 * Writes the new %esp in the TCB 
**/
#define SAVE_CONTEXT(current)         \
	__asm__ volatile                  \
	(                                 \
		"pushf\n\t"                   \
		"push %%eax\n\t"              \
		"push %%ebx\n\t"              \
		"push %%ecx\n\t"              \
		"push %%edx\n\t"              \
		"push %%esi\n\t"              \
		"push %%edi\n\t"              \
		"push %%ebp\n\t"              \
		"push %%ss\n\t"               \
		"push %%ds\n\t"               \
		"push %%es\n\t"               \
		"push %%fs\n\t"               \
		"push %%gs\n\t"               \
		"movl %%esp, %[esp]"          \
		: [esp] "=r"(current->esp)    \
		:                             \
		: "memory"                    \
	)


/* Context: Resume
 * resumes a thread from a context pointed to by the esp of the task
 * context that was previously saved by SAVE_CONTEXT
**/
#define DISPATCH(next)                \
	__asm__ volatile                  \
	(                                 \
		"movl %[esp], %%esp\n\t"      \
		"pop %%gs\n\t"                \
		"pop %%fs\n\t"                \
		"pop %%es\n\t"                \
		"pop %%ds\n\t"                \
		"pop %%ss\n\t"                \
		"popl %%ebp\n\t"              \
		"popl %%edi\n\t"              \
		"popl %%esi\n\t"              \
		"popl %%edx\n\t"              \
		"popl %%ecx\n\t"              \
		"popl %%ebx\n\t"              \
		"popl %%eax\n\t"              \
		"popf\n\t"                    \
		:                             \
		: [esp] "r"(next->esp)        \
		: "memory"                    \
	)


void
init_sched(void)
{
	__idle_task = kmalloc(sizeof(task_struct_t));
	__idle_task->status = NEW;
	__idle_task->callable = do_idle;
	__idle_task->arg = NULL;
	__idle_task->stack = kmalloc(0x100);
	__idle_task->esp = (uint32_t) __idle_task->stack + 0x100 - 1;

	#ifdef THREAD_DUMMY_CONTEXT_START
		SETUP_NEW_THREAD(__idle_task, sched_finalize_thread);
	#endif
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
	ts->esp = (uint32_t) ts->stack + THREAD_STACK_SIZE - 1;
	#ifdef THREAD_DUMMY_CONTEXT_START
		SETUP_NEW_THREAD(ts, sched_finalize_thread);
	#endif

	/* init fd table to nulls */
	memset(ts->fd_table, 0, NUM_MAX_FD * sizeof(FILE*));

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
	// __asm__ volatile("pushf":::"memory");
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
	if (__current_task->status != NEW)
		SAVE_CONTEXT(__current_task);
	
	if (__current_task->status == RUNNING)	
		__current_task->status = READY;

	/* FURTHER CODE MUST NOT MANIPULATE STACK IN NET EFFECT */

	/* PART 1: check all sleeping tasks to see if they are ready to be woken up */
	if (__waitq_head)
		service_waitq();

	/* PART 2: pick next task to be run */
	if (__current_task->status != NEW)
		__current_task = sched_select_next_rr();

	/* PART 3: switch threads */
	/* task switching is just infinite recurison over all the running threads */	
	__current_task->status = RUNNING;
	DISPATCH(__current_task);
}


void
do_timer(void)
{
	task_struct_t *waitq_itr = __waitq_head;
	if (__waitq_head)
	{
		do
		{
			++waitq_itr->utime;
			waitq_itr = waitq_itr->next;
		} while (waitq_itr != __waitq_head);
	}

	jiffies++;
	if (++__current_task->utime < _SCHED_TIMESLICE_TICKS_)
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
			waitq->sleep_time = 0;
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
__sleep_on(unsigned long systicks)
{
	__asm__ volatile("cli":::"memory");

	task_struct_t *curr_cpy = __current_task;

	// set metadata
	curr_cpy->status = BLOCKED;
	curr_cpy->utime = 0;
	curr_cpy->sleep_time = systicks;

	// remove from runq and add to waitq
	splice_outq(&__runq_head, curr_cpy);
	splice_inq(&__waitq_head, curr_cpy);

	// wait for block from incoming interrupt
	__asm__ volatile("sti":::"memory");
	while (curr_cpy->sleep_time != 0);
	
	// when resumed, reset time counters
	__current_task->utime = 0;
	__current_task->sleep_time = 0;
}


static void *
do_idle(void * arg)
{
	while (true)
		__asm__ volatile
			("nop":::"memory");

	// should never get here
	return NULL;
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
