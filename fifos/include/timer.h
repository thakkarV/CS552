#ifndef TIMER
#define TIMER

/* command and data ports for PIT */
#define PIT_CHANNEL0_DATA 0x40
#define PIT_COMMAND 0x43

/* base frequency and desired frequency in Hz */
#define PIT_BASE_FRQ 1193182
#define PIT_INT_FRQ 20

void init_pit(void);

#endif
