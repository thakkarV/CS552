#include <kmalloc.h>
#include <kvideo.h>
#include <stateful_cr.h>
#include <sys/kthread.h>
#include <sys/mutex.h>
#include <sys/types.h>
#include <sysutils.h>

// todo: move all this stuff ot the apporoporate header
#define NUM_THREADS 3

static kthread_mutex_t lock;


void *stateful_cr_thread1(void *arg)
{
	printf("<1> Arg = 0x%x\n", arg);
	int i;
	int j		= 0;
	int counter = 0;

	kthread_mutex_lock(&lock);
	while (1) {
		msleep(250);

		for (i = 0; i < 4; i++) {
			printf(" <1>:%d ", counter++);
		}
		printf("\n");

		if (++j == 3)
			break;
	}

	printf("Done <1>!\n");
	kthread_mutex_unlock(&lock);

	return (void *)0xBEEF;
}


void *stateful_cr_thread2(void *arg)
{
	printf("<2> Arg = 0x%x\n", arg);

	int i;
	int j		= 0;
	int counter = 0;

	kthread_mutex_lock(&lock);
	while (1) {
		msleep(500);

		for (i = 0; i < 5; i++) {
			printf(" <2>:%d ", counter++);
		}
		printf("\n");

		if (++j == 5)
			break;
	}

	printf("Done <2>!\n");
	kthread_mutex_unlock(&lock);

	return (void *)0xDEAD;
}


void *stateful_cr_thread3(void *arg)
{
	printf("<3> Arg = 0x%x\n", arg);

	int i;
	int j		= 0;
	int counter = 0;

	kthread_mutex_lock(&lock);
	while (1) {
		msleep(1500);

		for (i = 0; i < 4; i++) {
			printf(" <3>:%d ", counter++);
		}
		printf("\n");

		/* check if done with this thread */
		if (++j == 2)
			break;
	}

	printf("Done <3>!\n");
	kthread_mutex_unlock(&lock);
	return (void *)0xD00D;
}


void stateful_cr_register_routines(void)
{
	kthread_mutex_init(&lock);

	/* create threads and register with scheduler */
	tid_t thread1_id = kthread_create(stateful_cr_thread1, (void *)0xDEAD);
	if (thread1_id == -1)
		printf("\n    Could not create thread 1\n");
	else
		printf("\n    Thread 1 created with tid = %d\n", thread1_id);


	tid_t thread2_id = kthread_create(stateful_cr_thread2, (void *)0xBEEF);
	if (thread2_id == -1)
		printf("    Could not create thread 2\n");
	else
		printf("    Thread 2 created with tid = %d\n", thread2_id);

	tid_t thread3_id = kthread_create(stateful_cr_thread3, (void *)0xF00F);
	if (thread3_id == -1)
		printf("    Could not create thread 3\n");
	else
		printf("    Thread 3 created with tid = %d\n", thread3_id);
}
