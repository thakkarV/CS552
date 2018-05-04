#ifndef SYS_TIMER
#define SYS_TIMER

/* command and data ports for PIT */
#define PIT_CHANNEL0_DATA 0x40
#define PIT_COMMAND 0x43

/* base frequency and desired frequency in Hz */
#define PIT_BASE_FRQ 1193182

/* decreasing this decreases frequency */
#define PIT_INT_FRQ 50000

void init_pit(void);

#endif // SYS_TIMER
