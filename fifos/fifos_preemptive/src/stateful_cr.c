#include <stateful_cr.h>
#include <kvideo.h>
#include <kmalloc.h>
#include <threads.h>


void
stateful_cr_thread1(void)
{
	int i;
	int j = 0;
	int counter = 0;

	while (1)
	{
		msleep(500);		
		/* execute instructions in this thread */
		for (i = 0; i < 4; i++)
		{
			printf (" <1>:%d ", counter++);
		}
		printf ("\n");

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
	int j = 0;
	int counter = 0;

	while (1)
	{
		msleep(1000);		
		/* execute instructions in this thread */
		for (i = 0; i < 5; i++)
		{
			printf (" <2>:%d ", counter++);
		}
		printf ("\n");

		/* check if done with this thread */
		if (++j == 5)
			break;
	}

	printf("Done <2>!\n");
	return;
}


void
stateful_cr_thread3(void)
{
	int i;
	int j = 0;
	int counter = 0;

	while (1)
	{
		msleep(3000);
		/* execute instructions in this thread */
		for (i = 0; i < 4; i++)
		{
			printf (" <3>:%d ", counter++);
		}
		printf ("\n");

		/* check if done with this thread */
		if (++j == 2)
			break;
	}

	printf("Done <3>!\n");
	return;
}


void
stateful_cr_register_routines(void)
{
	/* create threads and register with scheduler */
	tid_t thread1_id = thread_create(stateful_cr_thread1);
	if (thread1_id == -1)
		printf("\n    Could not create thread 1\n");
	else
		printf("\n    Thread 1 created with tid = %d\n", thread1_id);


	tid_t thread2_id = thread_create(stateful_cr_thread2);
	if (thread2_id == -1)
		printf("    Could not create thread 2\n");
	else
		printf("    Thread 2 created with tid = %d\n", thread2_id);

	tid_t thread3_id = thread_create(stateful_cr_thread3);
	if (thread3_id == -1)
		printf("    Could not create thread 3\n");
	else
		printf("    Thread 3 created with tid = %d\n", thread3_id);
}
