#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

MODULE_LICENSE("GPL");

static int __init initialization_routine(void)
{
	printk("Hello, world!\n");
	return 0;
}

static void __exit cleanup_routine(void)
{
	printk("Unloading module!\n");
}

module_init(initialization_routine);
module_exit(cleanup_routine);
