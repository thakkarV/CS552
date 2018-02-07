#include <linux/kernel.h>	/* We're doing kernel work */
#include <linux/module.h>	/* Specifically, a module */
#include <linux/sched.h>
#include <linux/workqueue.h>
#include <linux/interrupt.h>	/* We want an interrupt */
#include <linux/errno.h>
#include <linux/proc_fs.h>

#include "kbd_irq_driver.h"

#define KBD_WORKQ_NAME "kbd_irq_driver.c"

static struct workqueue_struct * kbd_irq_workq;


static void got_char(void * scan_code)
{
	printk(KERN_INFO "Scan Code %x %s.\n",
		   (int)*((char *)scan_code) & 0x7F,
		   *((char *)scan_code) & 0x80 ? "Released" : "Pressed");
}


irqreturn_t irq_handler(int irq, void * dev_id)
{
	static int initialised = 0;
	static unsigned char scan_code;
	static struct work_struct task;
	unsigned char status;

	status = inb(0x64);
	scan_code = inb(0x60);

	if (initialised == 0)
	{
		INIT_WORK(&task, got_char, &scancode);
		initialised = 1;
	}
	else
	{
		PREPARE_WORK(&task, got_char, &scan_code);
	}

	queue_work(kbd_irq_workq, &task);

	return IRQ_HANDLED;
}


static int __init kbd_irq_init(void)
{
	// kbd_irq_dev_proc_operations.ioctl = kbd_irq_servicer;
	// kbd_irq_proc_entry = create_proc_entry("kbd_irq", 0444, NULL);
	// if(!kbd_irq_proc_entry)
	// {
	// 	printk("<1> Error creating /proc entry.\n");
	// 	return 1;
	// }

	// kbd_irq_proc_entry->proc_fops = &kbd_irq_dev_proc_operations;

	// setup work queue
	kbd_irq_workq = create_workqueue(KBD_WORKQ_NAME);
	free_irq(1, NULL);
	return request_irq(1, irq_handler, IRQF_SHARED, "kbd_irq_handler", (void *)(irq_handler));
}


static void __exit kbd_irq_exit(void)
{
	printk("<1> Dumping kbd_irq Module\n");
	// remove_proc_entry("kbd_irq", NULL);
	free_irq(1, (void *)(irq_handler));
}


static inline unsigned char
inb(unsigned short usPort)
{
	unsigned char uch;
	asm volatile( "inb %1,%0" : "=a" (uch) : "Nd" (usPort) );
	return uch;
}


static inline void
outb(unsigned char uch, unsigned short usPort)
{
	asm volatile( "outb %0,%1" : : "a" (uch), "Nd" (usPort) );
}


// kernel endpoints
module_init(kbd_irq_init); 
module_exit(kbd_irq_exit);

MODULE_LICENSE("GPL");
