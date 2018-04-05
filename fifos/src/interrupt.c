#include <interrupt.h>
#include <kportio.h>
#include <kstdlib.h>
#include <kvideo.h>
#include <types.h>

static struct IDTR idtr;
static struct IDTDesc IDT[ISR_COUNT];


void
init_pic(void)
{
	/* initialize master and slave PIC */
	outb(PIC1_CTRL, 0x11);
	outb(PIC2_CTRL, 0x11);

	/* set offsets for PICs */
	outb(PIC1_DATA, PIC1_OFFSET);
	outb(PIC2_DATA, PIC2_OFFSET);

	/* tell master PIC that slave PIC is at IRQ2 */
	outb(PIC1_DATA, 4);

	/* tell Slave PIC its cascade identity */
	outb(PIC2_DATA, 2);
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
	idtr.limit = (sizeof(struct IDTDesc) * ISR_COUNT) - 1;
	idtr.base = &IDT;

	// clear IDT and ensure all zeros
	memset(&IDT, 0, sizeof(struct IDTDesc) * ISR_COUNT);

	/* TIMER on IRQ-0 */
	idt_register_isr(32, &isr_32, 0x08, 0x8E);

	load_idt();
}


void
idt_register_isr(uint8_t isr_vector_num, void (* isr_vector)(void), uint16_t sel, uint8_t flg)
{
	IDT[isr_vector_num].offset_lo = (uint16_t) ((uint32_t) &isr_vector & 0xFFFF);
	IDT[isr_vector_num].selector = sel;
	IDT[isr_vector_num].zero = 0x00;
	IDT[isr_vector_num].flags = flg;
	IDT[isr_vector_num].offset_hi = (uint16_t) ((uint32_t) &isr_vector >> 16);
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
isr_timer(void)
{
	printf("time is up\n");
	eoi(0);
}
