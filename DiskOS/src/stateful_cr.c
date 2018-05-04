#include <sys/threads.h>
#include <stateful_cr.h>
#include <kvideo.h>
#include <kmalloc.h>

static kthread_mutex_t lock;

void *
stateful_cr_thread1(void * arg)
{
	printf("<1> Arg = 0x%x\n", arg);
	int i;
	int j = 0;
	int counter = 0;

	while (1)
	{
		// msleep(500);
		/* execute instructions in this thread */
		// kthread_mutex_lock(&lock);
		for (i = 0; i < 4; i++)
		{
			printf (" <1>:%d ", counter++);
		}
		printf ("\n");
		// kthread_mutex_unlock(&lock);

		/* check if done with this thread */
		if (++j == 3)
			break;
	}

	printf ("Done <1>!\n");
	return 0xBEEF;
}


void *
stateful_cr_thread2(void * arg)
{
	printf("<2> Arg = 0x%x\n", arg);
	
	int i;
	int j = 0;
	int counter = 0;

	while (1)
	{
		// msleep(1000);
		/* execute instructions in this thread */
		// kthread_mutex_lock(&lock);
		for (i = 0; i < 5; i++)
		{
			printf (" <2>:%d ", counter++);
		}
		printf ("\n");
		// kthread_mutex_unlock(&lock);

		/* check if done with this thread */
		if (++j == 5)
			break;
	}

	printf("Done <2>!\n");
	return 0xDEAD;
}


void *
stateful_cr_thread3(void * arg)
{
	printf("<3> Arg = 0x%x\n", arg);
	
	int i;
	int j = 0;
	int counter = 0;

	while (1)
	{
		// msleep(3000);
		// kthread_mutex_lock(&lock);
		/* execute instructions in this thread */
		for (i = 0; i < 4; i++)
		{
			printf (" <3>:%d ", counter++);
		}
		printf ("\n");
		// kthread_mutex_unlock(&lock);
		
		/* check if done with this thread */
		if (++j == 2)
			break;
	}

	printf("Done <3>!\n");
	return 0xD00D;
}


void
stateful_cr_register_routines(void)
{
	kthread_mutex_init(&lock);

	/* create threads and register with scheduler */
	tid_t thread1_id = kthread_create(stateful_cr_thread1, 0xDEAD);
	if (thread1_id == -1)
		printf("\n    Could not create thread 1\n");
	else
		printf("\n    Thread 1 created with tid = %d\n", thread1_id);


	tid_t thread2_id = kthread_create(stateful_cr_thread2, 0xBEEF);
	if (thread2_id == -1)
		printf("    Could not create thread 2\n");
	else
		printf("    Thread 2 created with tid = %d\n", thread2_id);

	tid_t thread3_id = kthread_create(stateful_cr_thread3, 0xF00F);
	if (thread3_id == -1)
		printf("    Could not create thread 3\n");
	else
		printf("    Thread 3 created with tid = %d\n", thread3_id);
}
