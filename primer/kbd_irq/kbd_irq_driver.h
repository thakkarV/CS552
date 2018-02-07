#ifndef KBD_IRQ_DRIVER
#define KBD_IRQ_DRIVER


static void got_char(void * scan_code);

irqreturn_t irq_handler(int irq, void * dev_id);

static int __init kbd_irq_init(void);

static void __exit kbd_irq_exit(void);

static inline unsigned char
inb(unsigned short usPort);

static inline void
outb(unsigned char uch, unsigned short usPort);

#endif