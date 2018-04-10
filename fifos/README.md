MEMEOS (fifos assignment)

Submission contains two directories
	fifos/fifos_cooperative
	fifos/fifos_preemptive

Each has its own makefile:

HOW TO RUN THE CODE:
You can either run the binary with -kernel flag or write the binary on the image file and then run the disk
To make the binary: 								make
To write binary to image:							make install
To make binary and run in qemu with -kernel flag:	make rrb (RECOMMENDED)
To make binary, write on image, and run in qemu:	make rrd

TLDR:
	in both fifos versions, just type "make rrb" to run the code


++++++++++ EXTRA CREDIT ++++++++++:

We have a pluggable scheduler. The user can write a function to preempt according to the policy of choice, and this function can be substituted for "sched_select_next_rr" in the scheduler routine.

We have malloc implemented

We have a thread_create function which serves as API call. So threads can be created dynamically at runtime, the only limit being the memory of the system.

CONTRIBUTIONS:

kmalloc, kvideo, kstdlib - Vijay
everything else - shared between Vijay and Karanraj
