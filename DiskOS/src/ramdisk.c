#include <ramdisk.h>
#include <sys/ufs.h>
#include <sched.h>
#include <stdlib.h>

static ufs_superblock_t *__superblk;
static inode_t          *__inode_array;
static uint8_t          *__blk_bitmap;
static ufs_dirblock_t   *__root_blk;

// bitmap ops
static void set_blk_bitmap(int, inode_status_t);
static bool get_blk_bitmap(int);

extern task_struct_t *__current_task;

void
init_ramdisk(void *fs_base_addr)
{
    /* initialize the whole 2 MB region to null */
    memset(fs_base_addr, 0, UFS_RAMDISK_SIZE);

    /* 0: INIT SUPERBLOCK */
    __superblk = (ufs_superblock_t *) fs_base_addr;
    __superblk->magic = UFS_HEADER_MAGIC;

    __superblk->block_size = UFS_BLOCK_SIZE;
    __superblk->num_blocks = UFS_NUM_MAX_BLOCKS;
    __superblk->num_free_blocks = UFS_NUM_MAX_BLOCKS - 1;
    
    __superblk->num_inodes = UFS_NUM_MAX_INODES;
    __superblk->num_free_inodes = UFS_NUM_MAX_INODES - 1;

    /* 1: INIT INODE ARRAY */
    __inode_array = (inode_t *) fs_base_addr + UFS_BLOCK_SIZE;

    /* 2: INIT INODE BITMAP */
    __blk_bitmap = (uint8_t *) __inode_array +
        (UFS_NUM_MAX_INODES * sizeof(inode_t));
    set_blk_bitmap(0, OCCUPIED);

    /* 3: INIT ROOT DIRECTORY BLOCK */
    __root_blk = (ufs_dirblock_t *) __blk_bitmap +
        + (UFS_BLOCK_SIZE * UFS_NUM_BITMAP_BLOCKS);
    __inode_array[0].type = DIR;
    // TODO: figure out dir names

    __superblk->inode_array = __inode_array;
    __superblk->blk_bitmap  = __blk_bitmap;
    __superblk->root_blk    = __root_blk;
}


static void
set_blk_bitmap(int inode_index, inode_status_t mark)
{
    if (inode_index < 0 || inode_index > UFS_NUM_MAX_INODES)
        return;
    
    uint8_t val = __blk_bitmap[inode_index / (sizeof(uint8_t) * 8)];
    uint8_t pos = __blk_bitmap[inode_index % (sizeof(uint8_t) * 8)];

    if (mark == FREE)
        val &= ~(1 << pos);
    else // mark OCCUPIED
        val |= (1 << pos);

    __blk_bitmap[inode_index / (sizeof(uint8_t) * 8)] = val;
}


static bool
is_blk_free(int inode_index)
{
    if (inode_index < 0 || inode_index > UFS_NUM_MAX_INODES)
        return false;
    
    uint8_t val = __blk_bitmap[inode_index / (sizeof(uint8_t) * 8)];
    uint8_t pos = __blk_bitmap[inode_index % (sizeof(uint8_t) * 8)];

    return ((val >> pos) & 1);
}