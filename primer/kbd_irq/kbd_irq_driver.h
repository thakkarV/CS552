#ifndef KBD_WAIT_DRIVER
#define KBD_WAIT_DRIVER

static int
kbd_irq_servicer(struct inode * inode,
				struct file * file,
				unsigned int cmd,
				unsigned long arg);

irqreturn_t irq_handler(int irq, void * dev_id);


static int __init kbd_irq_init(void);


static void __exit kbd_irq_exit(void);


static inline unsigned char
inb(unsigned short usPort);


static inline void
outb(unsigned char uch, unsigned short usPort);

static char
kbd_readkey(void);

#endif
