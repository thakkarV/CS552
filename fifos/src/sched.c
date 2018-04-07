#include <sched.h>
#include <kmalloc.h>
#include <thread.h>
#include <types.h>


static uint32_t thread_count;
static task_struct * __run_queue_head;
static task_struct * current;


static task_struct * sched_select_next(void);


void
sched_register_tcb(TCB * tcb)
{
	/* allocate task struct */
	task_struct * ts = kmalloc( sizeof(task_struct) );
	ts->tid = thread_count++;
	ts->tcb = tcb;
	ts->status = NEW;
	ts->priority = 0;

	/* add new task to run queue */
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
}


static task_struct *
sched_select_next(void)
{
	return current->next;
}


void
schedule(void)
{
	current->status = READY;
	SAVE_CONTEXT(&(current->tcb->esp));

	current = sched_select_next();

	if(current->status == NEW)
	{
		current->status = RUNNING;
	}
	else
	{
		current->status = RUNNING;
		DISPATCH(current);
		return;
	}
}
