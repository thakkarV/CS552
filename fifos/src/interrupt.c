#include <interrupt.h>
#include <kportio.h>
#include <kstdlib.h>
#include <kvideo.h>
#include <types.h>

static struct IDTR idtr;
static struct IDTDesc IDT[ISR_COUNT];
static long jiffies;

void
init_pic(void)
{
	/* ICW1 - initialize master and slave PIC */
	outb(PIC1_CTRL, 0x10);
	outb(PIC2_CTRL, 0x10);

	/* ICW2 - set offsets for PICs */
	outb(PIC1_DATA, PIC1_OFFSET);
	outb(PIC2_DATA, PIC2_OFFSET);

	/* ICW3 - tell master PIC that slave PIC is at IRQ2 */
	outb(PIC1_DATA, 0x4);

	/* ICW3 - tell Slave PIC its cascade identity */
	outb(PIC2_DATA, 0x2);
}


void
eoi(uint8_t irq)
{
	if (irq >= 8)
		outb(PIC2_CTRL, EOI);
	else
		outb(PIC1_CTRL, EOI);
	printf("eoi\n");
}


void
init_idt(void)
{
	short i = 0;
	idtr.limit = (sizeof(struct IDTDesc) * ISR_COUNT) - 1;
	idtr.base = &IDT[0];

	// TODO: is it ok to not have default isrs
	// clear IDT and ensure all zeros
	memset(&IDT, 0x0, sizeof(struct IDTDesc) * ISR_COUNT);


	set_trap_gate(  0, &isr_unimpl);
	set_trap_gate(  1, &isr_unimpl);
	set_trap_gate(  2, &isr_unimpl);
	set_system_gate(3, &isr_unimpl);
	set_system_gate(4, &isr_unimpl);
	set_system_gate(5, &isr_unimpl);
	set_trap_gate(  6, &isr_unimpl);
	set_trap_gate(  7, &isr_unimpl);
	set_trap_gate(  8, &isr_double_fault);
	set_trap_gate(  9, &isr_unimpl);
	set_trap_gate( 10, &isr_unimpl);
	set_trap_gate( 11, &isr_unimpl);
	set_trap_gate( 12, &isr_unimpl);
	set_trap_gate( 13, &isr_unimpl);
	set_trap_gate( 14, &isr_unimpl);
	set_trap_gate( 15, &isr_reserved);
	set_trap_gate( 16, &isr_unimpl);

	/* ISR 15 and 17-32 : hardware reserved */
	for (i = 17; i < 32; i++)
		set_trap_gate(i, &isr_reserved);

	/* ISR 32 : TIMER on IRQ-0 */
	set_intr_gate(32, &isr_timer);

	/* ISR 33 : KBD on IRQ-1 */
	set_intr_gate(33, &isr_kbd);

	load_idt();
}


void
load_idt(void)
{
	__asm__ volatile("lidt (idtr)");
}


void
IRQ_mask(uint8_t line)
{
	uint16_t port;
	uint8_t value;

	if(line < 8)
	{
		port = PIC1_DATA;
	}
	else
	{
		port = PIC2_DATA;
		line -= 8;
	}
	value = inb(port) | (1 << line);
	outb(port, value);
}


void
IRQ_unmask(uint8_t line)
{
	uint16_t port;
	uint8_t value;

	if(line < 8)
	{
		port = PIC1_DATA;
	}
	else
	{
		port = PIC2_DATA;
		line -= 8;
	}
	uint8_t foo;
	foo = 9;
	value = inb(port) & ~(1 << line);
	outb(port, value);
}


void
do_timer(void)
{
	jiffies++;
	printf("Timer Jiffies = %d\n", jiffies);
}


void
do_reserved(void)
{
	printf("Reserved\n");
}


void
do_unimpl(void)
{
	printf("unimplemented Trap\n");
}


void
do_double_fault(void)
{
	printf("DOUBLE FAULT\n");
}
