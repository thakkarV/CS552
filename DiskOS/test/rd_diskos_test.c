#include <sys/threads.h>
#include <sys/stdlib.h>
#include <sysutils.h>
#include <sys/ufs.h>
#include <stateful_cr.h>
#include <kvideo.h>
#include <kmalloc.h>
#include <ramdisk.h>
#include <test_rd.h>

/* we HAVE to use ramdisk */
#define USE_RAMDISK

/* uncomment to run test */
#define TEST1
#define TEST2
#define TEST3
#define TEST4
#define TEST5

/* uncomment to test single and double indirect pointers */
#define TEST_SINGLE_INDIRECT
#define TEST_DOUBLE_INDIRECT

/* test retval error codes */
#define TEST_SUCCESS 1
#define TEST_FAILURE 0

#define MAX_FILES 16
#define BLK_SZ 256					/* Block size */
#define DIRECT 8					/* Direct pointers in location attribute */
#define PTR_SZ 4					/* 32-bit [relative] addressing */
#define PTRS_PB  (BLK_SZ / PTR_SZ) 	/* Pointers per index block */

/* Insert a string for the pathname prefix here. For the ramdisk, it should be NULL */
#define PATH_PREFIX ""


static char pathname[80];
static char data1[DIRECT*BLK_SZ]; 			/* Largest data directly accessible */
static char data2[PTRS_PB*BLK_SZ];     		/* Single indirect data size */
static char data3[PTRS_PB*PTRS_PB*BLK_SZ]; 	/* Double indirect data size */
static char addr[PTRS_PB*PTRS_PB*BLK_SZ+1]; /* Scratchpad memory */


// /* helper functions */
// static void int2str(int source, char *res, int num_digits);
static void * thread0(void * arg);
static void * thread1(void * arg);
// static int pow(int num, int exponent);

void * run_tests(void * arg)
{
	int i, retval;
	int fd;
	int index_node_number;

	/* Some arbitrary data for our files */
	memset (data1, '1', sizeof (data1));
	memset (data2, '2', sizeof (data2));
	memset (data3, '3', sizeof (data3));

#ifdef TEST1

	/* Generate MAXIMUM regular files */
	char c = '0';
	for (i = 0; i < MAX_FILES + 1; i++)
	{ // go beyond the limit
		sprintf (pathname, PATH_PREFIX "/file");
		pathname[5] = c++;
		retval = rd_create (pathname);

		if (retval < 0)
		{
			printf("rd_create: File creation error! status: %d (%s)\n",
			retval, pathname);

			if (i != MAX_FILES)
				return TEST_FAILURE;
				// return TEST_FAILURE; // exit(EXIT_FAILURE);
		}

		memset (pathname, 0, 80);
	}   

	/* Delete all the files created */
	c = '0';	
	for (i = 0; i < MAX_FILES; i++)
	{ 
		sprintf (pathname, PATH_PREFIX "/file");
		pathname[5] = c++;		
		retval = rd_unlink (pathname);

		if (retval < 0)
		{
			printf ("unlink: File deletion error! status: %d\n", retval);
			return TEST_FAILURE;
			// return TEST_FAILURE; // exit(EXIT_FAILURE);
		}

		memset (pathname, 0, 80);
	}

#endif // TEST1

#ifdef TEST2

	/* ****TEST 2: LARGEST file size**** */

	/* Generate one LARGEST file */
	retval = rd_create (PATH_PREFIX "/bigfile");

	if (retval < 0)
	{
		printf ("creat: File creation error! status: %d\n", retval);

		return TEST_FAILURE;
		// return TEST_FAILURE; // exit(EXIT_FAILURE);
	}

	retval =  rd_open (PATH_PREFIX "/bigfile"); /* Open file to write to it */

	if (retval < 0)
	{
		printf ("open: File open error! status: %d\n", retval);

		return TEST_FAILURE;
		// return TEST_FAILURE; // exit(EXIT_FAILURE);
	}

	fd = retval;			/* Assign valid fd */

	/* Try writing to all direct data blocks */
	retval = rd_write (fd, data1, sizeof(data1));

	if (retval < 0)
	{
		printf ("write: File write STAGE1 error! status: %d\n",
		retval);

		return TEST_FAILURE;
		// return TEST_FAILURE; // exit(EXIT_FAILURE);
	}

#ifdef TEST_SINGLE_INDIRECT

	/* Try writing to all single-indirect data blocks */
	retval = rd_write (fd, data2, sizeof(data2));

	if (retval < 0)
	{
		printf ("write: File write STAGE2 error! status: %d\n",
		retval);

		return TEST_FAILURE;
		// return TEST_FAILURE; // exit(EXIT_FAILURE);
	}

#ifdef TEST_DOUBLE_INDIRECT

	/* Try writing to all double-indirect data blocks */
	retval = rd_write (fd, data3, sizeof(data3));

	if (retval < 0)
	{
		printf ("write: File write STAGE3 error! status: %d\n",
		retval);

		return TEST_FAILURE;
		// return TEST_FAILURE; // exit(EXIT_FAILURE);
	}

#endif // TEST_DOUBLE_INDIRECT

#endif // TEST_SINGLE_INDIRECT

	printf("test2 ok\n");

#endif // TEST2

#ifdef TEST3

	/* ****TEST 3: Seek and Read file test**** */
	retval = rd_lseek (fd, 0, SEEK_SET);	/* Go back to the beginning of your file */

	if (retval < 0)
	{
		printf("lseek: File seek error! status: %d\n",
		retval);

		return TEST_FAILURE; // exit(EXIT_FAILURE);
	}

	char t[2];

	/* Try reading from all direct data blocks */
	retval = rd_read (fd, t, 1);

	if (retval < 0)
	{
		printf("read: File read STAGE1 error! status: %d\n",
		retval);

		return TEST_FAILURE; // exit(EXIT_FAILURE);
	}

	t[1] = '\0';

	/* Should be all 1s here... */
	// printf ("Data at addr: %s\n", t);

#ifdef TEST_SINGLE_INDIRECT

	retval = rd_lseek (fd, 5000, SEEK_SET);	/* Go back to the beginning of your file */

	/* Try reading from all single-indirect data blocks */
	retval = rd_read (fd, t, 1);

	if (retval < 0) {
		printf("read: File read STAGE2 error! status: %d\n",
		retval);

		return TEST_FAILURE; // exit(EXIT_FAILURE);
	}

	t[1] = '\0';
	/* Should be all 2s here... */
	printf ("Data at addr: %s\n", t);

#ifdef TEST_DOUBLE_INDIRECT

	/* Try reading from all double-indirect data blocks */
	retval = rd_read (fd, addr, sizeof(data3));

	if (retval < 0)
	{
		printf("read: File read STAGE3 error! status: %d\n",
		retval);

		return TEST_FAILURE; // exit(EXIT_FAILURE);
	}
	/* Should be all 3s here... */
	// printf ("Data at addr: %s\n", addr);

#endif // TEST_DOUBLE_INDIRECT

#endif // TEST_SINGLE_INDIRECT

	/* Close the bigfile */
	retval = rd_close(fd);

	if (retval < 0)
	{
		printf("close: File close error! status: %d\n",
		retval);

		return TEST_FAILURE; // exit(EXIT_FAILURE);
	}

	/* Remove the biggest file */

	retval = rd_unlink (PATH_PREFIX "/bigfile");

	if (retval < 0)
	{
		printf("unlink: /bigfile file deletion error! status: %d\n",
		retval);

		return TEST_FAILURE; // exit(EXIT_FAILURE);
	}

#endif // TEST3

#ifdef TEST4

	/* ****TEST 4: Make directory and read directory entries**** */
	retval = rd_mkdir (PATH_PREFIX "/dir1");

	if (retval < 0)
	{
		printf("mkdir: Directory 1 creation error! status: %d\n",
		retval);

		return TEST_FAILURE; // exit(EXIT_FAILURE);
	}

	retval = rd_mkdir (PATH_PREFIX "/dir1/dir2");

	if (retval < 0)
	{
		printf("mkdir: Directory 2 creation error! status: %d\n",
		retval);

		return TEST_FAILURE; // exit(EXIT_FAILURE);
	}

	retval = rd_mkdir (PATH_PREFIX "/dir1/dir3");

	if (retval < 0)
	{
		printf("mkdir: Directory 3 creation error! status: %d\n",
		retval);

		return TEST_FAILURE; // exit(EXIT_FAILURE);
	}

	#ifdef USE_RAMDISK
	retval =  rd_open (PATH_PREFIX "/dir1"); /* Open directory file to read its entries */

	if (retval < 0)
	{
		printf("open: Directory open error! status: %d\n",
		retval);

		return TEST_FAILURE; // exit(EXIT_FAILURE);
	}

	fd = retval;			/* Assign valid fd */

	memset (addr, 0, sizeof(addr)); /* Clear scratchpad memory */

	char readdirbuf[16];

	int count = 0;
	while (!(retval = rd_readdir (fd, readdirbuf)))
	{
		/* 0 indicates end-of-file */
		if (++count == 3)
			break;

		if (retval < 0)
		{
			printf("readdir: Directory read error! status: %d\n",
			retval);
			return TEST_FAILURE; // exit(EXIT_FAILURE);
		}

		// index_node_number = atoi(&addr[14]);
		index_node_number = (&readdirbuf[14]);
		printf("%s\n", readdirbuf);
		printf ("Contents at addr: [%s,%s]\n", readdirbuf, index_node_number);
	
	}

#endif // USE_RAMDISK
#endif // TEST4

// TODO: test this fork "replacement"
#ifdef TEST5

	/* Create two threads and run rd create on both */
	tid_t thread0_id = kthread_create(thread0, NULL);
	if (thread0_id == -1)
		printf("    Could not create thread 0\n");
	else
		printf("    Thread 0 created with tid = %d\n", thread0_id);

	tid_t thread1_id = kthread_create(thread1, NULL);
	if (thread1_id == -1)
		printf("\n    Could not create thread 1\n");
	else
		printf("\n    Thread 1 created with tid = %d\n", thread1_id);

	__asm__ volatile("sti");

#endif // TEST5

	return TEST_SUCCESS;
}


static void * thread0(void * arg)
{
	int i, retval;

	/* Generate 300 regular files */
	for (i = 0; i < 1; i++)
	{ 
		// TODO: replace sprinft
		sprintf (pathname, PATH_PREFIX "/file_t0");

		retval = rd_create (pathname);

		if (retval < 0)
		{
			printf("(thread0) create: File creation error! status: %d\n", 
			retval);
			return TEST_FAILURE; // exit(EXIT_FAILURE);
		}

		memset (pathname, 0, 80);
	}

	msleep(200);

	int fd;
	fd = rd_open("/");

	char readdirbuf[16];
	int count = 0;
	int index_node_number;
	while (!(retval = rd_readdir (fd, readdirbuf)))
	{ /* 0 indicates end-of-file */

		if (++count == 3)
			break;

		if (retval < 0)
		{
			printf("readdir: Directory read error! status: %d\n",
			retval);
			return TEST_FAILURE; // exit(EXIT_FAILURE);
		}

		// index_node_number = atoi(&addr[14]);
		index_node_number = (&readdirbuf[14]);
		printf("%s\n", readdirbuf);
		printf ("Contents at addr: [%s,%s]\n", readdirbuf, index_node_number);
	
	}

	return 0;
}


static void * thread1(void * arg)
{
	int i, retval;

	// msleep(100);
	/* Generate 300 regular files */
	for (i = 0; i < 1; i++)
	{ 
		sprintf (pathname, PATH_PREFIX "/file_t1");

		retval = rd_create (pathname);

		if (retval < 0)
		{
			printf("(thread1) create: File creation error! status: %d\n", 
			retval);
			return TEST_FAILURE; // exit(EXIT_FAILURE);
		}

		memset (pathname, 0, 80);
	}

	return 0;
}


/**
 * 
 * HELPER FUNCTIONS
 * 
 */


// static int pow(int num, int exponent)
// {
// 	int res = 1;
// 	int i;
// 	for (i = 0; i < exponent; ++i)
// 		res *= num;

// 	return res;
// }


// static void int2str(int source, char * res, int num_digits)
// {
// 	int i;
// 	for (i = 0; i < num_digits; ++i)
// 		res[i] = (source%pow(10,num_digits-i) / pow(10,num_digits-i-1)) + 0x30;

// 	res[num_digits] = '\0';
// }
