#include <kmalloc.h>
#include <kvideo.h>
#include <ramdisk.h>
#include <stateful_cr.h>
#include <sys/kthread.h>
#include <sys/stdlib.h>
#include <sys/ufs.h>
#include <test_rd.h>

/* we HAVE to use ramdisk */
#define USE_RAMDISK

/* uncomment to run test */
#define TEST1
#define TEST2
#define TEST3
#define TEST4
#define TEST5

static void *rd_test_thread0(void *);
static void *rd_test_thread1(void *);

/* uncomment to test single and double indirect pointers */
#define TEST_SINGLE_INDIRECT
#define TEST_DOUBLE_INDIRECT

/* test retval error codes */
#define TEST_SUCCESS (void *)1
#define TEST_FAILURE (void *)0

#define MAX_FILES 16
#define BLK_SZ 256 /* Block size */
#define DIRECT 8 /* Direct pointers in location attribute */
#define PTR_SZ 4 /* 32-bit [relative] addressing */
#define PTRS_PB (BLK_SZ / PTR_SZ) /* Pointers per index block */

/* Insert a string for the pathname prefix here. For the ramdisk, it should be
 * NULL */
#define PATH_PREFIX ""


static char pathname[80];
static char data1[DIRECT * BLK_SZ]; /* Largest data directly accessible */
static char data2[PTRS_PB * BLK_SZ]; /* Single indirect data size */
static char data3[PTRS_PB * PTRS_PB * BLK_SZ]; /* Double indirect data size */
static char addr[PTRS_PB * PTRS_PB * BLK_SZ + 1]; /* Scratchpad memory */

// /* helper functions */
// static void int2str(int source, char *res, int num_digits);
// static int pow(int num, int exponent);


// int main()
// {
// 	int status;

// 	status = run_tests();
// 	if (status != TEST_SUCCESS)
// 		printf("rd tests failed\n");
// 	else
// 		printf("rd tests passed\n");

// 	return 0;
// }


void *run_tests(void *arg)
{
	int i, retval;
	int fd;

	/* Some arbitrary data for our files */
	memset(data1, '1', sizeof(data1));
	memset(data2, '2', sizeof(data2));
	memset(data3, '3', sizeof(data3));

#ifdef TEST1

	/* Generate MAXIMUM regular files */
	char c = '0';
	for (i = 0; i < MAX_FILES + 1; i++) { // go beyond the limit
		sprintf(pathname, PATH_PREFIX "/file");
		pathname[5] = c++;
		retval		= rd_create(pathname);

		if (retval < 0) {
			printf("rd_create: File creation error! status: %d (%s)\n", retval,
				pathname);

			if (i != MAX_FILES)
				return TEST_FAILURE;
			// return TEST_FAILURE; // exit(EXIT_FAILURE);
		}

		memset(pathname, 0, 80);
	}

	/* Delete all the files created */
	c = '0';
	for (i = 0; i < MAX_FILES; i++) {
		sprintf(pathname, PATH_PREFIX "/file");
		pathname[5] = c++;
		retval		= rd_unlink(pathname);

		if (retval < 0) {
			printf("unlink: File deletion error! status: %d\n", retval);
			return TEST_FAILURE;
			// return TEST_FAILURE; // exit(EXIT_FAILURE);
		}

		memset(pathname, 0, 80);
	}

#endif // TEST1

#ifdef TEST2

	/* ****TEST 2: LARGEST file size**** */

	/* Generate one LARGEST file */
	retval = rd_create(PATH_PREFIX "/bigfile");

	if (retval < 0) {
		printf("creat: File creation error! status: %d\n", retval);

		return TEST_FAILURE;
		// return TEST_FAILURE; // exit(EXIT_FAILURE);
	}

	retval = rd_open(PATH_PREFIX "/bigfile"); /* Open file to write to it */

	if (retval < 0) {
		printf("open: File open error! status: %d\n", retval);

		return TEST_FAILURE;
		// return TEST_FAILURE; // exit(EXIT_FAILURE);
	}

	fd = retval; /* Assign valid fd */

	/* Try writing to all direct data blocks */
	retval = rd_write(fd, data1, sizeof(data1));

	if (retval < 0) {
		printf("write: File write STAGE1 error! status: %d\n", retval);

		return TEST_FAILURE;
		// return TEST_FAILURE; // exit(EXIT_FAILURE);
	}

#ifdef TEST_SINGLE_INDIRECT

	/* Try writing to all single-indirect data blocks */
	retval = rd_write(fd, data2, sizeof(data2));

	if (retval < 0) {
		printf("write: File write STAGE2 error! status: %d\n", retval);

		return TEST_FAILURE;
		// return TEST_FAILURE; // exit(EXIT_FAILURE);
	}

#ifdef TEST_DOUBLE_INDIRECT

	/* Try writing to all double-indirect data blocks */
	retval = rd_write(fd, data3, sizeof(data3));

	if (retval < 0) {
		printf("write: File write STAGE3 error! status: %d\n", retval);

		return TEST_FAILURE;
		// return TEST_FAILURE; // exit(EXIT_FAILURE);
	}

#endif // TEST_DOUBLE_INDIRECT

#endif // TEST_SINGLE_INDIRECT

	printf("test2 ok\n");

#endif // TEST2

#ifdef TEST3

	/* ****TEST 3: Seek and Read file test**** */
	retval
		= rd_lseek(fd, 0, SEEK_SET); /* Go back to the beginning of your file */

	if (retval < 0) {
		printf("lseek: File seek error! status: %d\n", retval);

		return TEST_FAILURE; // exit(EXIT_FAILURE);
	}

	/* Try reading from all direct data blocks */
	retval = rd_read(fd, addr, sizeof(data1));

	if (retval < 0) {
		printf("read: File read STAGE1 error! status: %d\n", retval);

		return TEST_FAILURE; // exit(EXIT_FAILURE);
	}
	/* Should be all 1s here... */
	printf("Data at addr: %s\n", addr);

#ifdef TEST_SINGLE_INDIRECT

	/* Try reading from all single-indirect data blocks */
	retval = rd_read(fd, addr, sizeof(data2));

	if (retval < 0) {
		printf("read: File read STAGE2 error! status: %d\n", retval);

		return TEST_FAILURE; // exit(EXIT_FAILURE);
	}
	/* Should be all 2s here... */
	printf("Data at addr: %s\n", addr);

#ifdef TEST_DOUBLE_INDIRECT

	/* Try reading from all double-indirect data blocks */
	retval = rd_read(fd, addr, sizeof(data3));

	if (retval < 0) {
		printf("read: File read STAGE3 error! status: %d\n", retval);

		return TEST_FAILURE; // exit(EXIT_FAILURE);
	}
	/* Should be all 3s here... */
	printf("Data at addr: %s\n", addr);

#endif // TEST_DOUBLE_INDIRECT

#endif // TEST_SINGLE_INDIRECT

	/* Close the bigfile */
	retval = rd_close(fd);

	if (retval < 0) {
		printf("close: File close error! status: %d\n", retval);

		return TEST_FAILURE; // exit(EXIT_FAILURE);
	}

	/* Remove the biggest file */

	retval = rd_unlink(PATH_PREFIX "/bigfile");

	if (retval < 0) {
		printf("unlink: /bigfile file deletion error! status: %d\n", retval);

		return TEST_FAILURE; // exit(EXIT_FAILURE);
	}

#endif // TEST3

#ifdef TEST4

	/* ****TEST 4: Make directory and read directory entries**** */
	retval = rd_mkdir(PATH_PREFIX "/dir1");

	if (retval < 0) {
		printf("mkdir: Directory 1 creation error! status: %d\n", retval);

		return TEST_FAILURE; // exit(EXIT_FAILURE);
	}

	retval = rd_mkdir(PATH_PREFIX "/dir1/dir2");

	if (retval < 0) {
		printf("mkdir: Directory 2 creation error! status: %d\n", retval);

		return TEST_FAILURE; // exit(EXIT_FAILURE);
	}

	retval = rd_mkdir(PATH_PREFIX "/dir1/dir3");

	if (retval < 0) {
		printf("mkdir: Directory 3 creation error! status: %d\n", retval);

		return TEST_FAILURE; // exit(EXIT_FAILURE);
	}

#ifdef USE_RAMDISK
	retval = rd_open(
		PATH_PREFIX "/dir1"); /* Open directory file to read its entries */

	if (retval < 0) {
		printf("open: Directory open error! status: %d\n", retval);

		return TEST_FAILURE; // exit(EXIT_FAILURE);
	}

	fd = retval; /* Assign valid fd */

	memset(addr, 0, sizeof(addr)); /* Clear scratchpad memory */

	while ((retval = rd_readdir(fd, addr))) { /* 0 indicates end-of-file */

		if (retval < 0) {
			printf("readdir: Directory read error! status: %d\n", retval);
			return TEST_FAILURE; // exit(EXIT_FAILURE);
		}

		// index_node_number = atoi(&addr[14]);
		void *index_node_number = (void *)(addr + 14);
		printf("Contents at addr: [%s,%x]\n", addr, index_node_number);
	}

#endif // USE_RAMDISK
#endif // TEST4

// TODO: test this fork "replacement"
#ifdef TEST5

	/* Create two threads and run rd create on both */
	tid_t thread0_id = kthread_create(rd_test_thread0, NULL);
	if (thread0_id == -1)
		printf("    Could not create thread 0\n");
	else
		printf("    Thread 0 created with tid = %d\n", thread0_id);

	tid_t thread1_id = kthread_create(rd_test_thread1, NULL);
	if (thread1_id == -1)
		printf("\n    Could not create thread 1\n");
	else
		printf("\n    Thread 1 created with tid = %d\n", thread1_id);

#endif // TEST5

	return TEST_SUCCESS;
}


static void *rd_test_thread0(void *arg)
{
	int i, retval;

	/* Generate 300 regular files */
	for (i = 0; i < 300; i++) {
		// TODO: replace sprinft
		sprintf(pathname, PATH_PREFIX "/file_t0_%d", i);

		retval = rd_create(pathname);

		if (retval < 0) {
			printf(
				"(thread0) create: File creation error! status: %d\n", retval);
			return TEST_FAILURE; // exit(EXIT_FAILURE);
		}

		memset(pathname, 0, 80);
	}

	return NULL;
}


static void *rd_test_thread1(void *arg)
{
	int i, retval;

	/* Generate 300 regular files */
	for (i = 0; i < 300; i++) {
		sprintf(pathname, PATH_PREFIX "/file_t1_%d", i);

		retval = rd_create(pathname);

		if (retval < 0) {
			printf(
				"(thread1) create: File creation error! status: %d\n", retval);
			return TEST_FAILURE; // exit(EXIT_FAILURE);
		}

		memset(pathname, 0, 80);
	}

	return NULL;
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
