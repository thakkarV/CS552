#include <ramdisk.h>
#include <ufs.h>

static void * __rdisk_base_addr;
static void * __superblk;
static void * __inode_bitmap;
static fs_dirblock_t * __fs_root_blk;
