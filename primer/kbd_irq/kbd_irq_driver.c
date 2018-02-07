#include <linux/kernel.h>	/* We're doing kernel work */
#include <linux/module.h>	/* Specifically, a module */
#include <linux/sched.h>
#include <linux/workqueue.h>
#include <linux/interrupt.h>	/* We want an interrupt */
#include <linux/errno.h>
#include <linux/proc_fs.h>
#include <linux/wait.h>

#include "kbd_irq_driver.h"

#define KBD_IOCTL_READKEY _IOR(0, 1, char)

static wait_queue_head_t wait_q;
static struct file_operations  kbd_irq_dev_proc_operations;
static struct proc_dir_entry * kbd_irq_proc_entry;


static int
kbd_irq_servicer(struct inode * inode,
				struct file * file,
				unsigned int cmd,
				unsigned long arg)
{
	char c;
	switch (cmd)
	{
		case KBD_IOCTL_READKEY:
		{
			printk("KBD_IOCTL_READKEY called\n");
			wait_event(wait_q);
			c = kbd_readkey();
			copy_to_user((char *)arg, &c, sizeof(char));
			printk("<1> Copied (%x) to userspace\n", c);
			break;
		}
		default:
		{
			return -EINVAL;
			break;
		}
	}
	return 0;
}


irqreturn_t irq_handler(int irq, void * dev_id)
{
	wake_up(&wait_q);
	return IRQ_HANDLED;
}


static int __init kbd_irq_init(void)
{
	int ret;
	printk("<1> Loading interrupt based KBD.\n");

	// now point the ioctl vector to the service routine for the kbd_test driver
	kbd_irq_dev_proc_operations.ioctl = kbd_irq_servicer;
	kbd_irq_proc_entry = create_proc_entry("kbd_irq", 0444, NULL);
	if(!kbd_irq_proc_entry)
	{
		printk("<1> Error creating /proc entry.\n");
		return 1;
	}

	kbd_irq_proc_entry->proc_fops = &kbd_irq_dev_proc_operations;

	init_waitqueue_head(&wait_q);
	ret = request_irq(1, irq_handler, IRQF_SHARED, "kbd_irq_handler", (void *)(irq_handler));
	printk("<1> Registered handler with %x cookie. %d.\n", (unsigned int) irq_handler, ret);
	return ret;
}


static void __exit kbd_irq_exit(void)
{
	printk("<1> Dumping kbd_irq Module\n");
	remove_proc_entry("kbd_irq", NULL);
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


static char
kbd_readkey(void)
{
	static char scancode[128] = "\0\e1234567890-=\177\tqwertyuiop[]\n\0asdfghjkl;'`\0\\zxcvbnm,./\0*\0 \0\0\0\0\0\0\0\0\0\0\0\0\000789-456+1230.\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";
	while (( c = inb( 0x60 ) ) & 0x80 );
	return scancode[(int)c];
}


// kernel endpoints
module_init(kbd_irq_init); 
module_exit(kbd_irq_exit);

MODULE_LICENSE("GPL");
