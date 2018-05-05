#include <sys/threads.h>
#include <stateful_cr.h>
#include <kvideo.h>
#include <kmalloc.h>

// todo: move all this stuff ot the apporoporate header
#define NUM_THREADS 3

static kthread_mutex_t lock;

// bakery algorithm
static int number[NUM_THREADS];
static bool choosing[NUM_THREADS];

// function declarations
static int max(int, int, int);


void *
stateful_cr_thread1(void * arg)
{
	// printf("<1> Arg = 0x%x\n", arg);
	int i;
	int j = 0;
	int k;
	int counter = 0;

	while (1)
	{
		// msleep(500);
		/* execute instructions in this thread */
		// kthread_mutex_lock(&lock);
		choosing[0] = 1;
		number[0] = max(number[0], number[1], number[2]) + 1;
		choosing[0] = 0;

		for (k = 0; k < NUM_THREADS; ++k)
		{
			while (choosing[k]);
			while ( (number[k]!=0) && ( (number[k]<number[0]) || (number[k]==number[0] && k < 0) ) );
		}

		for (i = 0; i < 4; i++)
		{
			printf (" <1>:%d ", counter++);
		}
		printf ("\n");
		// kthread_mutex_unlock(&lock);
		number[0] = 0;

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
	// printf("<2> Arg = 0x%x\n", arg);
	
	int i;
	int j = 0;
	int k;
	int counter = 0;
	uint32_t dummy;

	while (1)
	{
		// msleep(1000);
		/* execute instructions in this thread */
		// kthread_mutex_lock(&lock);
		choosing[1] = 1;
		number[1] = max(number[0], number[1], number[2]) + 1;
		choosing[1] = 0;
		for (k = 0; k < NUM_THREADS; ++k)
		{
			while (choosing[k]);
			while ( (number[k]!=0) && ( (number[k]<number[1]) || (number[k]==number[1] && k < 1) ) );
		}

		for (i = 0; i < 5; i++)
		{
			printf (" <2>:%d ", counter++);
		}
		printf ("\n");
		// kthread_mutex_unlock(&lock);
		number[1] = 0;

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
	// printf("<3> Arg = 0x%x\n", arg);
	
	int i;
	int j = 0;
	int k;
	int counter = 0;

	while (1)
	{
		// msleep(3000);
		// kthread_mutex_lock(&lock);
		/* execute instructions in this thread */
		choosing[2] = 1;
		number[2] = max(number[0], number[1], number[2]) + 1;
		choosing[2] = 0;
		for (k = 0; k < NUM_THREADS; ++k)
		{
			while (choosing[k]);
			while ( (number[k]!=0) && ( (number[k]<number[2]) || (number[k]==number[2] && k < 2) ) );
		}

		for (i = 0; i < 4; i++)
		{
			printf (" <3>:%d ", counter++);
		}
		printf ("\n");
		// kthread_mutex_unlock(&lock);
		number[2] = 0;
		
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
	/* initialize to false */
	int i = 0;
	for (int i = 0; i < NUM_THREADS; ++i)
	{
		choosing[i] = 0;
		number[i] = 0;
	}

	// kthread_mutex_init(&lock);

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


/**
 * @brief returns the max value among three values
 * 
 * @param a compare item 0
 * @param b compare item 1
 * @param c compare item 2
 */
static int
max(int a, int b, int c)
{
	if (a > b)
	{
		if (c > a)
		{
			return c;
		}
		else
		{
			return a;
		}
	}
	else
	{
		if (c > b)
		{
			return c;
		}
		else
		{
			return b;
		}
	}
}
