#include <linux/kernel.h>	/* We're doing kernel work */
#include <linux/module.h>	/* Specifically, a module */
#include <linux/sched.h>
#include <linux/workqueue.h>
#include <linux/interrupt.h>	/* We want an interrupt */
#include <asm/io.h>

#define KBD_WORKQ_NAME "kbd_irq_driver.c"

static struct workqueue_struct * kbd_irq_workq;


static void got_char(void * scan_code)
{
	printk(KERN_INFO "Scan Code %x %s.\n",
		   (int)*((char *)scan_code) & 0x7F,
		   *((char *)scan_code) & 0x80 ? "Released" : "Pressed");
}


irqreturn_t irq_handler(int irq, void * dev_id, struct pt_regs * regs)
{
	static int initialised = 0;
	static unsigned char scancode;
	static struct work_struct task;
	unsigned char status;

	status = inb(0x64);
	scan_code = inb(0x60);

	if (initialised == 0)
	{
		INIT_WORK(&task, got_char, &scan_code);
		initialised = 1;
	}
	else
	{
		PREPARE_WORK(&task, got_char, &scan_code);
	}

	queue_work(kbd_irq_workq, &task);

	return IRQ_HANDLED;
}


static int __init kbd_int_init(void)
{
	kbd_irq_dev_proc_operations.ioctl = kbd_irq_servicer;
	kbd_irq_proc_entry = create_proc_entry("kbd_irq", 0444, NULL);
	if(!kbd_irq_proc_entry)
	{
		printk("<1> Error creating /proc entry.\n");
		return 1;
	}

	kbd_irq_proc_entry->proc_fops = &kbd_irq_dev_proc_operations;

	// setup work queue
	kbd_irq_workq = create_workqueue(KBD_WORKQ_NAME);
	free_irq(1, NULL);
	return request_irq(1, irq_handler, SA_SHIRQ, "kbd_irq_handler", (void *)(irq_handler));
}


static void __exit kbd_int_exit(void)
{
	free_irq(1, (void *)(irq_handler));
}


// kernel endpoints
module_init(kbd_int_init); 
module_exit(kbd_int_exit); 


MODULE_LICENSE("GPL");
