#ifndef STATEFUL_CR
#define STATEFUL_CR

/* routines run on threads */
void *stateful_cr_thread1(void *);
void *stateful_cr_thread2(void *);
void *stateful_cr_thread3(void *);

/* registers routines with correspoinding thread */
void stateful_cr_register_routines(void);

#endif // STATEFUL_CR
