#include <threads.h>
#include <sched.h>
#include <kmalloc.h>
#include <types.h>

static unsigned int num_threads = 0;

int
thread_create(void * (* callable) (void *))
{
	__asm__ volatile("cli"::);

	if (num_threads == MAX_THREADCOUNT)
		return -1;

	/* initalize tcb to be allocated to a thread */
	TCB * tcb = kmalloc(sizeof(TCB));
	tcb->stack = kmalloc(THREAD_STACK_SIZE);
	tcb->callable = callable;
	tcb->esp = tcb->stack + THREAD_STACK_SIZE - 1;

	/* register this tcb to a task struct */
	sched_register_tcb(tcb);

	__asm__ volatile("sti"::);
	return 0;
}


void
thread_exit(void)
{
	TCB * tcb = sched_unregsiter_tcb();
	kfree(tcb->stack);
	bfree(tcb);
	num_threads--;
}
