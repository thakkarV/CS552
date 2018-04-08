#include <sched.h>
#include <kmalloc.h>
#include <threads.h>
#include <types.h>


static uint32_t __thread_count;
static task_struct_t * __run_queue_head;
static task_struct_t * __current_task;


static task_struct_t * sched_select_next_rr(void);


tid_t
sched_register_thread(void * (*callable) (void *), void * args)
{
	if (__thread_count == MAX_THREADCOUNT)
		return -1;

	/* allocate task struct */
	task_struct_t * ts = kmalloc(sizeof(task_struct_t));

	/* init task metadata */
	ts->tid = __thread_count++;
	ts->status = NEW;
	ts->priority = 0;

	/* Setup thread environment */
	ts->callable = callable;
	ts->args = args;
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

	return ts->tid;
}


void
sched_finalize_thread(void)
{
	kfree(__current_task->stack);
	__current_task->status = EXITED;
	__thread_count--;
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
	/* Set current to ready, and save its machine context */
	__current_task->status = READY;

	SAVE_CONTEXT(__current_task->esp);
	/* FURTHER CODE MUST NOT MANIPULATE STACK IN NET EFFECT */

	/* Pick next task to be run */
	__current_task = sched_select_next_rr();

	/* Nimble handinling required if the task has never run before */
	if(__current_task->status == NEW)
	{
		__current_task->status = RUNNING;
	}
	else
	{
		/* Task switching is just infinite reucurison over all the running threads */
		__current_task->status = RUNNING;
		DISPATCH(__current_task->esp);
		return;
	}
}
