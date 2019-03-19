#include <sched.h>
#include <sys/ufs.h>
#include <sys/vfs.h>

static task_struct_t *__current_task;

// return a file discriptor for the current task if available, -1 otherwise
int get_avail_fd(void)
{
	int i;
	for (i = 0; i < NUM_MAX_FD; i++) {
		if (!(__current_task->fd_table[i]))
			return i;
	}
	return -1;
}
