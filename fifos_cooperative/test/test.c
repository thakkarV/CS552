#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#define SAVE_CONTEXT(task_esp_ptr) \
__asm__ volatile( \
	"movl %%ecx, %0" \
	: "=r" (task_esp_ptr)\
	: \
	: "memory" \
)


#define DISPATCH(task_esp_ptr) \
__asm__ volatile( \
	"movl %0, %%ecx" \
	: \
	: "r" (task_esp_ptr) \
	: "memory" \
)

#define START_NEW_THREAD(func, args, esp, exit_routine)\
	__asm__ volatile(                                       \
	"movl %0, %%esp\n\t"                                    \
	"pushl %%ebp\n\t"                                       \
	"movl %%esp, %%eax\n\t" \
	"addl $4, %%eax\n\t" \
	"movl %%eax, %%ebp\n\t"                                 \
	"movl %%esp, %%ebp\n\t"                                 \
	"pushl %1\n\t"                                          \
	"pushl %2\n\t"                                          \
	"jmp %3\n\t"                                            \
	:                                                       \
	: "rm" (esp), "rm" (args), "rm" (exit_routine), "rm" (func) \
	: "eax", "memory")


#define START_NEW_NOARG_THREAD(func, esp, exit_routine)\
	__asm__ volatile(                                       \
	"movl %0, %%esp\n\t"                                    \
	"pushl %1\n\t"                                                     \
	"jmp %2\n\t"                                            \
	:                                                       \
	: "rm" (esp), "rm" (exit_routine), "rm" (func) \
	: "memory")

typedef struct ts
{
	unsigned long int reg;
} ts;


void thread_noarg(void)
{
	printf("eklflsaijfas;ogk\n");
}


void thread(void *args)
{
	printf("in thread\n");
	printf("%x\n", (args));
	return;
}


void thread_exit(void)
{
	printf("exiting thread\n");
}


int main(int argc, char const * argv [])
{
	ts ts1;
	ts1.reg = 0;

	SAVE_CONTEXT(ts1.reg);
	printf("ECX = %x\n", ts1.reg);

	ts ts2;
	ts2.reg = 0;
	DISPATCH(ts2.reg);

	ts ts3;
	ts3.reg = 0x1337;
	SAVE_CONTEXT(ts3.reg);
	printf("ECX = %x\n", ts3.reg);

	// __asm__ volatile("\tpushl %[exit_addr]\n"\
	// 					: \
	// 					: [exit_addr] "r" (retloc) );

	// __asm__ volatile("\tpushl %[thread_exit_addr]\n"\
	// 					: \
	// 					: [thread_exit_addr] "r" (thread_exit) );

	// __asm__ volatile("\tjmp %[thread_addr]\n" \
	// 					: \
	// 					: [thread_addr] "r" (thread) );

	// __asm__ volatile("retloc:" :::);


	// void *foo = malloc( 1024 );

	// ts *ptr_args;
	// ptr_args = (ts*) malloc(sizeof(ts));
	// ptr_args->reg = 1997;

	// printf("Arg addr = %x\n", ptr_args);

	// START_NEW_THREAD(thread, ptr_args, ((char*) foo) + 1023, thread_exit);

	// START_NEW_NOARG_THREAD(thread_noarg, (char*)foo + 1023, thread_exit);

	return 0;
}
