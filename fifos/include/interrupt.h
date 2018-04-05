#ifndef INTERRUPT
#define INTERRUPT

#include "types.h"

/* set interrupt flag */
#define sti() __asm__ ("sti"::) 

/* clear interrupt flag */
#define cli() __asm__ ("cli"::) 

/* Master PIC ports and offset */
#define PIC1_CTRL    0x20
#define PIC1_DATA    0x21
#define PIC1_OFFSET  0X20

/* Slave PIC ports and offset */
#define PIC2_CTRL    0xA0
#define PIC2_DATA    0xA1
#define PIC2_OFFSET  0X28

#define EOI          0x20

#define ISR_COUNT    0x100

void init_pic(void);
void init_idt(void);
void load_idt(void);
void idt_register_isr(uint8_t, void (*)(void), uint16_t, uint8_t);

void IRQ_mask(uint8_t line);
void IRQ_unmask(uint8_t line);

void eoi(uint8_t);

/* ASM HANDLERS */
extern void isr_32(void);


/* C HANDLERS */
void isr_timer(void);


struct IDTDesc
{
	uint16_t offset_lo;     // offset bits 0..15
	uint16_t selector;      // a code segment selector in GDT or LDT
	uint8_t zero;           // unused, set to 0
	uint8_t flags;          // type and attributes, see below
	uint16_t offset_hi;     // offset bits 16..31
} __attribute__((packed));


struct IDTR
{
	uint16_t limit;
	struct IDTDesc * base;
} __attribute__((packed));

#endif // INTERRUPT
