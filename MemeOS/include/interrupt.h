#ifndef INTERRUPT
#define INTERRUPT

#include <sys/types.h>

/* set interrupt flag */
#define sti() __asm__("sti" ::: "memory")

/* clear interrupt flag */
#define cli() __asm__("cli" ::: "memory")

void init_pic(void);
void init_idt(void);

void irq_mask(uint8_t line);
void irq_unmask(uint8_t line);

#endif // INTERRUPT
