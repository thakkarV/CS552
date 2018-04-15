#ifndef INTERRUPT
#define INTERRUPT

#include <types.h>

/* set interrupt flag */
#define sti() __asm__ ("sti"::) 

/* clear interrupt flag */
#define cli() __asm__ ("cli"::) 

/* Master PIC ports and offset */
#define PIC1_CTRL    0x20
#define PIC1_DATA    0x21
#define PIC1_OFFSET  0x20

/* Slave PIC ports and offset */
#define PIC2_CTRL    0xA0
#define PIC2_DATA    0xA1
#define PIC2_OFFSET  0x28

#define EOI          0x20

#define ISR_COUNT    0x100


#define iret() __asm__ ("iret"::)

#define _set_gate(gate_addr, type, dpl, addr) \
__asm__ ("movw %%dx, %%ax\n\t" \
	"movw %0, %%dx\n\t" \
	"movl %%eax, %1\n\t" \
	"movl %%edx, %2" \
	: \
	: "i" ((short) (0x8000+(dpl << 13) + (type << 8))), \
	"o" (*((char *) (gate_addr))), \
	"o" (*(4 + (char *) (gate_addr))), \
	"d" ((char *) (addr)), "a" (0x00080000))

#define set_intr_gate(n, addr) \
	_set_gate(&IDT[n], 14, 0, addr)

#define set_trap_gate(n, addr) \
	_set_gate(&IDT[n], 15, 0, addr)

#define set_system_gate(n, addr) \
	_set_gate(&IDT[n], 15, 3, addr)


void init_pic(void);
void init_idt(void);
void load_idt(void);
void idt_register_isr(uint8_t, void (*)(void), uint16_t, uint8_t);

void irq_mask(uint8_t line);
void irq_unmask(uint8_t line);


/* ASM HANDLERS */
extern void isr_divide_error(void);
extern void isr_debug(void);
extern void isr_nmi(void);
extern void isr_int3(void);
extern void isr_overflow(void);
extern void isr_bounds(void);
extern void isr_invalid_op(void);
extern void isr_device_unavailable(void);
extern void isr_double_fault(void);
extern void isr_coprocessor_segment_overrun(void);
extern void isr_invalid_tss(void);
extern void isr_segment_not_present(void);
extern void isr_stack_segment_fault(void);
extern void isr_general_protection_fault(void);
extern void isr_page_fault(void);
extern void isr_reserved(void);
extern void isr_x87_fpu_except(void);

extern void isr_timer(void);
extern void isr_kbd(void);

/* C HANDLERS */
void do_divide_error(unsigned long, long);
void do_debug(unsigned long, long);
void do_nmi(unsigned long, long);
void do_int3(unsigned long, long);
void do_overflow(unsigned long, long);
void do_bounds(unsigned long, long);
void do_invalid_op(unsigned long, long);
void do_device_unavailable(unsigned long, long);
void do_double_fault(unsigned long, long);
void do_coprocessor_segment_overrun(unsigned long, long);
void do_invalid_tss(unsigned long, long);
void do_segment_not_present(unsigned long, long);
void do_stack_segment_fault(unsigned long, long);
void do_general_protection_fault(unsigned long, long);
void do_page_fault(unsigned long, long);
void do_reserved(unsigned long, long);
void do_x87_fpu_except(unsigned long, long);


/* Inspired by Torwalds Kernel 0.99 */
void die(char *, unsigned long, long);


struct IDTDesc
{
	/* Flags layout
		+---+---+---+---+---+---+---+---+
		| P |  DPL  | S |    GateType   |
		+---+---+---+---+---+---+---+---+
	 */
	uint16_t offset_lo;     // offset bits 0..15
	uint16_t selector;      // a code segment selector in GDT or LDT
	uint8_t zero;           // unused, set to 0
	uint8_t flags;          // type and attributes
	uint16_t offset_hi;     // offset bits 16..31
} __attribute__((packed));


struct IDTR
{
	uint16_t limit;
	struct IDTDesc * base;
} __attribute__((packed));

#endif // INTERRUPT
