#include <linux/init.h> // the __init __exit macros
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/proc_fs.h> // ioctl entry point interface
#include <linux/interrupt.h>
#include <asm/uaccess.h>

#include "kbd_bw_driver.h"

#define KBD_IOCTL_TEST _IOW(0, 0, struct kbd_action)
#define KBD_IOCTL_READKEY _IOR(0, 1, char)

MODULE_LICENSE("GPL");

static struct file_operations  kbd_bw_dev_proc_operations;
static struct proc_dir_entry * kbd_bw_proc_entry;

static int
kbd_bw_servicer(struct inode * inode,
				struct file * file,
				unsigned int cmd,
				unsigned long arg)
{
	struct kbd_action key_event;
	disable_irq(1);
	switch (cmd)
	{
		case KBD_IOCTL_TEST:
		{
			copy_from_user(&key_event, (struct kbd_action *)arg, sizeof(struct kbd_action));
			printk("<1> ioctl: call to KBD_IOCTL_TEST (%c,%d)!\n", key_event.key, key_event.status);
			printk("<1> KBD Module : kbd_test_ioctl_servicer called with IOCTL_TEST.\n");
			break;
		}
		case KBD_IOCTL_READKEY:
		{
			char c = kbd_readkey();
			copy_to_user((char *)arg, &c, sizeof(char));
			printk("<1> KBD Module : kbd_test_ioctl_servicer called with KBD_IOCTL_READKEY.\n");
			printk("<1> Copied (%x) to userspace\n", c);
			break;
		}
		default:
		{
			return -EINVAL;
			break;
		}
	}

	enable_irq(1);

	return 0;
}


static char
kbd_readkey(void)
{
	char c;
	static char scancode[128] = "\0\e1234567890-=\177\tqwertyuiop[]\n\0asdfghjkl;'`\0\\zxcvbnm,./\0*\0 \0\0\0\0\0\0\0\0\0\0\0\0\000789-456+1230.\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";

	while( !(inb( 0x64 ) & 0x1) 
		|| ( ( c = inb( 0x60 ) ) & 0x80 ) );

	return scancode[(int)c];
}


static int
__init kbd_bw_init(void)
{
	printk("<1> Loading busywait based KBD.\n");

	// now point the ioctl vector to the service routine for the kbd_test driver
	kbd_bw_dev_proc_operations.ioctl = kbd_bw_servicer;
	kbd_bw_proc_entry = create_proc_entry("kbd_bw", 0444, NULL);
	if(!kbd_bw_proc_entry)
	{
		printk("<1> Error creating /proc entry.\n");
		return 1;
	}

	kbd_bw_proc_entry->proc_fops = &kbd_bw_dev_proc_operations;
	return 0;
}


static void
__exit kbd_bw_exit(void)
{
	printk("<1> Dumping kbd_bw Module\n");
	remove_proc_entry("kbd_bw", NULL);
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
module_init(kbd_bw_init); 
module_exit(kbd_bw_exit); 
