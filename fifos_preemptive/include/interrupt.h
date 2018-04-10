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

void IRQ_mask(uint8_t line);
void IRQ_unmask(uint8_t line);

void eoi(uint8_t);

/* ASM HANDLERS */
extern void isr_timer(void);
extern void isr_reserved(void);
extern void isr_unimpl(void);
extern void isr_double_fault(void);
extern void isr_kbd(void);

/* C HANDLERS */
void do_reserved(void);
void do_unimpl(void);
void do_double_fault(void);

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
