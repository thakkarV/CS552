#ifndef KBD_BW_DRIVER
#define KBD_BW_DRIVER

struct kbd_action
{
	char key;
	int status;
};


static int
kbd_bw_servicer(struct inode * inode,
				struct file * file,
				unsigned int cmd,
				unsigned long arg);

static char
kbd_readkey(void);

static int
__init kbd_bw_init(void);

static void
__exit kbd_bw_exit(void);

static inline unsigned char
inb(unsigned short usPort);

static inline void
outb(unsigned char uch, unsigned short usPort);

#endif