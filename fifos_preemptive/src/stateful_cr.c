#include <stateful_cr.h>
#include <sched.h>
#include <kvideo.h>
#include <kmalloc.h>
#include <threads.h>


void
stateful_cr_thread1(void)
{
	int i;
	static int j;
	int counter = 0;

	while (1)
	{
		/* execute instructions in this thread */
		for (i = 0; i < 10; i++)
		{
			printf ("<1> : %d", counter++);
		}
		printf ("\n");

		/* yield and switch to another thread */
		schedule();

		/* check if done with this thread */
		if (++j == 3)
			break;
	}

	printf ("Done <1>!\n");
	return;
}


void
stateful_cr_thread2(void)
{
	int i;
	static int j;
	int counter = 0;

	while (1)
	{
		/* execute instructions in this thread */
		for (i = 0; i < 5; i++)
		{
			printf ("<2> : %d", counter++);
		}
		printf ("\n");

		/* yield and switch to another thread */
		schedule();

		/* check if done with this thread */
		if (++j == 5)
			break;
	}

	printf("Done <2>!\n");
	return;
}


void
stateful_cr_register_routines(void)
{
	/* create threads and register with scheduler */
	tid_t thread1_id = thread_create(stateful_cr_thread1);
	if (thread1_id == -1)
	{
		printf("Could not create thread 1\n");
	}

	tid_t thread2_id = thread_create(stateful_cr_thread2);
	if (thread2_id == -1)
	{
		printf("Could not create thread 2\n");
	}

	/* run threads */
}
