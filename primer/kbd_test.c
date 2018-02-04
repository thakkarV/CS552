#include <linux/init.h> // the __init __exit macros
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/proc_fs.h> // ioctl entry point interface
#include <asm/uaccess.h>


MODULE_LICENSE("GPL");

#define IOCTL_TEST _IOW(0, 6, struct ioctl_test_t)

static struct file_operations  pseudo_dev_proc_operations;
static struct proc_dir_entry * proc_entry;
struct ioctl_test_t
{
	int field1;
	char field2;
};


static int kbd_test_ioctl_servicer(struct inode * inode, struct file * file,
							unsigned int cmd, unsigned long arg);


static int kbd_test_ioctl_servicer(struct inode * inode, struct file * file,
				unsigned int cmd, unsigned long arg)
{
	struct ioctl_test_t ioc;

	switch (cmd)
	{
		case IOCTL_TEST:
		{
			copy_from_user(&ioc, (struct ioctl_test_t *)arg, sizeof(struct ioctl_test_t));
			printk("<1> ioctl: call to IOCTL_TEST (%d,%c)!\n", ioc.field1, ioc.field2);
			my_printk ("kbd_test Service called.\n");
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


static int __init init_kbd_test(void)
{
	printk("<1> Loading kbd_test Module\n");

	// now point the ioctl vector to the service routine for the kbd_test driver
	pseudo_dev_proc_operations.ioctl = kbd_test_ioctl_servicer;
	proc_entry = create_proc_entry("kbd_test", 0444, NULL);
	if(!proc_entry)
	{
		printk("<1> Error creating /proc entry.\n");
		return 1;
	}

	proc_entry->proc_fops = &pseudo_dev_proc_operations;
}


static int __exit exit_kbd_test(void)
{
	printk("<1> Dumping kbd_test Module\n");
	remove_proc_entry("kbd_test", NULL);

	return;
}


// kernel endpoints
module_init(initialization_routine); 
module_exit(cleanup_routine); 
