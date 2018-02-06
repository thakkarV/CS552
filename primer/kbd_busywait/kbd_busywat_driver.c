#include <linux/init.h> // the __init __exit macros
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/proc_fs.h> // ioctl entry point interface
#include <asm/uaccess.h>

MODULE_LICENSE("GPL");

struct kbd_action
{
	char key;
	int status;
}

#define KBD_IOCTL_TEST _IOW(0, 0, struct ioctl_test_t)
#define KBD_IOCTL_READKEY _IOR(0, 1, struct kbd_action)

static int
kbd_bw_servicer(struct inode * inode,
				struct file * file,
				unsigned int cmd,
				unsigned long arg)
{
	switch (cmd)
	{
		case KBD_IOCTL_TEST:
		{
			struct kbd_action kbd_test;
			copy_from_user(&kbd_test, (struct kbd_action *)arg, sizeof(struct kbd_action));
			printk("<1> ioctl: call to KBD_IOCTL_TEST (%d,%c)!\n", kbd_test.key, kbd_test.status);
			printk("<1> KBD Module : kbd_test_ioctl_servicer called with IOCTL_TEST.\n");
			break;
		}
		case KBD_IOCTL_READKEY:
		{
			struct kbd_action key_event;
			char c = kbd_readkey();
			copy_to_user((struct kbd_action *)arg, &key_event, sizeof(struct kbd_action));
			printk("<1> KBD Module : kbd_test_ioctl_servicer called with KBD_IOCTL_READKEY.\n");
			printk("<1> Copied (%d,%c) to userspace\n", key_event.key, key_event.status);
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


static char
kbd_readkey(void)
{
	char c;
	static char scancode[128] = "\0\e1234567890-=\177\tqwertyuiop[]\n\0asdfghjkl;'`\0\\zxcvbnm,./\0*\0 \0\0\0\0\0\0\0\0\0\0\0\0\000789-456+1230.\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";

	/* Poll keyboard status register at port 0x64 checking bit 0 to see if
	* output buffer is full. We continue to poll if the msb of port 0x60
	* (data port) is set, as this indicates out-of-band data or a release
	* keystroke
	*/
	while( !(inb( 0x64 ) & 0x1) 
		|| ( ( c = inb( 0x60 ) ) & 0x80 ) );

	return scancode[(int)c];
}


static int
__init kbd_bw_init(void)
{
	printk("<1> Loading busywait based KBD.\n")

	// now point the ioctl vector to the service routine for the kbd_test driver
	pseudo_dev_proc_operations.ioctl = kbd_bw_getchar_servicer;
	proc_entry = create_proc_entry("kbd_bw", 0444, NULL);
	if(!proc_entry)
	{
		printk("<1> Error creating /proc entry.\n");
		return 1;
	}

	proc_entry->proc_fops = &kbd_bw_servicer;
	return 0;
}



static void
__exit kbd_bw_exit(void)
{
	printk("<1> Dumping kbd_test Module\n");
	remove_proc_entry("kbd_test", NULL);
}


char
my_getchar(void)
{
	char c;
	static char scancode[128] = "\0\e1234567890-=\177\tqwertyuiop[]\n\0asdfghjkl;'`\0\\zxcvbnm,./\0*\0 \0\0\0\0\0\0\0\0\0\0\0\0\000789-456+1230.\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";

	/* Poll keyboard status register at port 0x64 checking bit 0 to see if
	* output buffer is full. We continue to poll if the msb of port 0x60
	* (data port) is set, as this indicates out-of-band data or a release
	* keystroke
	*/
	while( !(inb( 0x64 ) & 0x1) || ( ( c = inb( 0x60 ) ) & 0x80 ) );

	return scancode[(int)c];
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
