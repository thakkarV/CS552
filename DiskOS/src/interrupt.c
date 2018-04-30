#include <interrupt.h>
#include <kportio.h>
#include <kstdlib.h>
#include <kvideo.h>
#include <types.h>

static struct IDTR idtr;
static struct IDTDesc IDT[ISR_COUNT];

extern void do_timer(void);

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

	for (int i = 1; i < 16; ++i)
	{
		irq_mask(i);
	}
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

	set_trap_gate(  0, &isr_divide_error);
	set_trap_gate(  1, &isr_debug);
	set_trap_gate(  2, &isr_nmi);
	set_system_gate(3, &isr_int3);
	set_system_gate(4, &isr_overflow);
	set_system_gate(5, &isr_bounds);
	set_trap_gate(  6, &isr_invalid_op);
	set_trap_gate(  7, &isr_device_unavailable);
	set_trap_gate(  8, &isr_double_fault);
	set_trap_gate(  9, &isr_coprocessor_segment_overrun);
	set_trap_gate( 10, &isr_invalid_tss);
	set_trap_gate( 11, &isr_segment_not_present);
	set_trap_gate( 12, &isr_stack_segment_fault);
	set_trap_gate( 13, &isr_general_protection_fault);
	set_trap_gate( 14, &isr_page_fault);
	set_trap_gate( 15, &isr_reserved);
	set_trap_gate( 16, &isr_x87_fpu_except);
 
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
irq_mask(uint8_t line)
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
irq_unmask(uint8_t line)
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
	value = inb(port) & ~(1 << line);
	outb(port, value);
}


void
die(char * msg, unsigned long esp_ptr, long error_code)
{
	long * esp = (long *) esp_ptr;
	printf(FG_COLOR_DARK_RED BG_COLOR_WHITE);
	printf("%s: 0x%x\n", msg, error_code & 0xFFFF);
	printf("EIP:    0x%x:0x%x\n"\
		   "EFLAGS:    0x%x\n"\
		   "ESP:    0x%x:0x%x\n",
		esp[1], esp[0], esp[2], esp[4], esp[3]
	);

	__asm__ volatile ("cli"::);
	while (1);
}


void do_divide_error(unsigned long esp, long error_code)
{
	die("Divide By Zero", esp, error_code);
}


void
do_debug(unsigned long esp, long error_code)
{
	die("Debug", esp, error_code);
}


void 
do_nmi(unsigned long esp, long error_code)
{
	die("NMI", esp, error_code);
}


void do_int3(unsigned long esp, long error_code)
{
	die("Int 3 Break Point", esp, error_code);
}


void
do_overflow(unsigned long esp, long error_code)
{
	die("Overflow\n", esp, error_code);
}


void
do_bounds(unsigned long esp, long error_code)
{
	die("Bounds Range Exceeded", esp, error_code);
}


void
do_invalid_op(unsigned long esp, long error_code)
{
	die("Invalid Operand", esp, error_code);
}


void
do_device_unavailable(unsigned long esp, long error_code)
{
	die("Device Not Available\n", esp, error_code);
}


void
do_double_fault(unsigned long esp, long error_code)
{
	die("DOUBLE FAULT", esp, error_code);
}


void
do_coprocessor_segment_overrun(unsigned long esp, long error_code)
{
	die("Coprocessor Segment Overrun", esp, error_code);
}


void 
do_invalid_tss(unsigned long esp, long error_code)
{
	die("Invalid TSS", esp, error_code);
}

void
do_segment_not_present(unsigned long esp, long error_code)
{
	die("segment not present", esp, error_code);
}


void
do_stack_segment_fault(unsigned long esp, long error_code)
{
	die("stack segment", esp, error_code);
}


void
do_general_protection_fault(unsigned long esp, long error_code)
{
	die("General Protection Fault", esp, error_code);
}


void
do_page_fault(unsigned long esp, long error_code)
{
	die("Page Fault", esp, error_code);
}


void
do_reserved(unsigned long esp, long error_code)
{
	die("Reserved", esp, error_code);
}


void
do_x87_fpu_except(unsigned long esp, long error_code)
{
	die("x87 FPU Exception", esp, error_code);
}
