#include <ramdisk.h>
#include <sys/stdlib.h>
#include <sys/threads.h>
#include <sys/ufs.h>
#include <sys/vfs.h>
#include <sched.h>

static ufs_superblock_t *__superblk;
static inode_t          *__inode_array;
static uint8_t          *__blk_bitmap;
static ufs_dirblock_t   *__root_blk;
static kthread_mutex_t  *fs_header_lock;

// bitmap ops
static void set_blk_bitmap(int, inode_status_t);
static bool get_blk_bitmap(int);

// helpers
static inode_t * get_file_inode(char *, ufs_dirblock_t *);

extern task_struct_t *__current_task;

void
init_ramdisk(void *fs_base_addr)
{
    /* 0: initialize the whole 2 MB region to null */
    memset(fs_base_addr, 0, UFS_DISK_SIZE);

    /* 1: INIT SUPERBLOCK */
    __superblk = (ufs_superblock_t *) fs_base_addr;
    __superblk->magic = UFS_HEADER_MAGIC;

    __superblk->block_size = UFS_BLOCK_SIZE;
    __superblk->num_blocks = UFS_NUM_MAX_BLOCKS;
    __superblk->num_free_blocks = UFS_NUM_MAX_BLOCKS - 1;
    
    __superblk->num_inodes = UFS_NUM_MAX_INODES;
    __superblk->num_free_inodes = UFS_NUM_MAX_INODES - 1;

    /* 2: INIT INODE ARRAY */
    __inode_array = (inode_t *) fs_base_addr + UFS_BLOCK_SIZE;

    /* 3: INIT INODE BITMAP */
    __blk_bitmap = (uint8_t *) __inode_array +
        (UFS_NUM_MAX_INODES * sizeof(inode_t));
    set_blk_bitmap(0, OCCUPIED);

    /* 4: INIT ROOT DIRECTORY BLOCK */
    __root_blk = (ufs_dirblock_t *) __blk_bitmap +
        (UFS_BLOCK_SIZE * UFS_NUM_BITMAP_BLOCKS);
    __inode_array[0].type = DIR;
    // TODO: figure out dir names

    __superblk->inode_array = __inode_array;
    __superblk->blk_bitmap  = __blk_bitmap;
    __superblk->root_blk    = __root_blk;
}


int
rd_open(char * path)
{
    // see if this proc can open any more files
    int fd = get_avail_fd();
    if (fd == -1)
        return -1;
    
    // check path validity
    inode_t * file_inode = get_file_inode(path, __root_blk);
	
    // file path was invalid
    if (!file_inode)
        return -1;

    // create control block file object and set metadata
    FILE *file_obj = kmalloc(sizeof(FILE));
    file_obj->fd = fd;
    file_obj->inode_ptr = file_inode;
    file_obj->path = (char *) kmalloc(strlen(path));
    strcpy(path, file_obj->path);
    file_obj->seek_head = 0;

    // add the file obj in the control block at the opended fd
    __current_task->fd_table[fd] = file_obj;
    return fd;
}


int
rd_close(int fd)
{
    FILE *file_obj = __current_task->fd_table[fd];
    if (!file_obj)
        return -1;
    
    kfree(file_obj->path);
    kfree(file_obj);
    __current_task->fd_table[fd] = NULL;
    return 0;
}


int
rd_read(int fd, char * buf, int num_bytes)
{
	FILE * file_obj = __current_task->fd_table[fd];
	inode_t * file_inode = file_obj->inode_ptr;	
	
    // check if valid fd
    if (!file_obj)
		return EBADF;
	
	// bad buffer
	if (!buf)
		return EBADBUF;

	// file overread
    if (num_bytes < 0 || num_bytes > file_inode->size - file_obj->seek_head)
		return EOVERREAD;

    int read_counter = 0;

	// first we set up the scanner read head blk pointers and indexes
	ufs_datablock_t ***double_blk_ptr = file_inode->double_indirect_block_ptr;
	ufs_datablock_t  **single_blk_ptr = file_inode->indirect_block_ptr;
	ufs_datablock_t  **direct_blk_ptr = file_inode->direct_block_ptrs;
	ufs_datablock_t   *blk_ptr;
	int double_blk_idx;
	int single_blk_idx;
	int blk_num    = file_obj->seek_head / UFS_BLOCK_SIZE;
	int blk_offset = file_obj->seek_head % UFS_BLOCK_SIZE;
	
	// SETUP PONITERS AND INDEXES FOR READING
	// reading from direct mapped region
	if (blk_num < UFS_NUM_DIRECT_PTRS)
	{
		// set initial indexes
		double_blk_idx = 0;
		single_blk_idx = 0;

		// set initial pointers
		blk_ptr = file_inode->direct_block_ptrs[file_obj->seek_head % UFS_BLOCK_SIZE];
	}

	// reading from singly differed region
	else if (blk_num < UFS_NUM_DIRECT_PTRS + UFS_NUM_PTRS_PER_BLK)
	{
		// set initial indexes
		double_blk_idx = 0;
		single_blk_idx = (blk_num - UFS_NUM_DIRECT_PTRS) % UFS_NUM_PTRS_PER_BLK;
		
		// set initial pointers
		blk_ptr = single_blk_ptr[single_blk_idx];
	}

	// rading from doubly deffered region
	else
	{
		// set initial indexes		
		double_blk_idx = (blk_num - UFS_NUM_DIRECT_PTRS) / UFS_NUM_PTRS_PER_BLK;
		single_blk_idx = (blk_num - UFS_NUM_DIRECT_PTRS) % UFS_NUM_PTRS_PER_BLK;
		
		// set initial pointers
		single_blk_ptr = double_blk_ptr[double_blk_idx];
		blk_ptr = single_blk_ptr[single_blk_idx];
	}

	// START READING
	goto start_reading;
	while (double_blk_idx < UFS_NUM_PTRS_PER_BLK)
	{
		while (single_blk_idx < UFS_NUM_PTRS_PER_BLK)
		{
			start_reading:
			while (blk_offset < UFS_BLOCK_SIZE)
			{
				*buf++ = blk_ptr->data[blk_offset++];
				if (++read_counter == num_bytes)
					break;
			}

			if (++read_counter == num_bytes)
				break;

			blk_offset = 0;
			blk_num++;

			// handle direct pointer edge case
			if (blk_num < UFS_NUM_DIRECT_PTRS)
			{
				blk_ptr = direct_blk_ptr[blk_num];
			}
			else
			{
				single_blk_idx++;
				blk_ptr = single_blk_ptr[single_blk_idx];
			}
		}

		if (++read_counter == num_bytes)
			break;

		double_blk_idx++;
		single_blk_ptr = double_blk_ptr[double_blk_idx];

		single_blk_idx = 0;
		blk_ptr = single_blk_ptr[single_blk_idx];		
	}

    return 0;
}


// int
// rd_write(int fd, char * buf, int num_bytes_to_write)
// {
//     // check if valid fd
//     if (!__current_task->fd_table[0])
//     {
//         printf("invalid fd=%d\n", fd);
//         return -1;
//     }

//     // check if valid buf
//     if (!buf)
//     {
//         printf("invalid data input buf\n");
//         return -1;
//     }

//     // check if valid num_bytes_to_write
//     if (num_bytes_to_write < 0 || num_bytes_to_write > UFS_NUM_MAX_BLOCKS*UFS_BLOCK_SIZE)
//     {
//         printf("invalid num_bytes_to_write=%d\n", num_bytes_to_write);
//         return -1;
//     }

//     // get file struct
//     FILE * file_obj = __current_task->fd_table[fd];
//     size_t seek_head = file_obj->seek_head;

//     inode_t * file_inode = file_obj->inode_ptr;

//     // block number where the required byte is
//     int block_num;

//     // start reading from first block
//     int num_bytes_written = 0;
//     while (num_bytes_written != num_bytes_to_write)    // or !EOF
//     {
//         block_num = (seek_head + num_bytes_written)/UFS_BLOCK_SIZE;

//         if (block_num < NUM_DIRECT_PTRS)
//         {
//             printf("blk num %d, char num %d\n", num_bytes_written/UFS_BLOCK_SIZE, num_bytes_written%UFS_BLOCK_SIZE);
//             (file_inode->direct_block_ptrs[num_bytes_written/UFS_BLOCK_SIZE])->data[num_bytes_written%UFS_BLOCK_SIZE] = *(buf + num_bytes_written);
//         }
//         else if (block_num < NUM_DIRECT_PTRS + NUM_PTRS_PER_BLK)
//         {
//             printf("ptr num %d, char num %d\n", block_num-NUM_DIRECT_PTRS, num_bytes_written%UFS_BLOCK_SIZE);
//             (*(file_inode->indirect_block_ptr) + (block_num-NUM_DIRECT_PTRS))->data[num_bytes_written%UFS_BLOCK_SIZE] = *(buf + num_bytes_written);
//         }
//         else if (block_num < NUM_DIRECT_PTRS + NUM_PTRS_PER_BLK + NUM_PTRS_PER_BLK*NUM_PTRS_PER_BLK)
//         {
//             printf("indir ptr num %d, ptr num %d, char num %d\n", (block_num-NUM_DIRECT_PTRS-NUM_PTRS_PER_BLK)/NUM_PTRS_PER_BLK, (block_num-NUM_DIRECT_PTRS-NUM_PTRS_PER_BLK)%NUM_PTRS_PER_BLK, num_bytes_written%UFS_BLOCK_SIZE);
//             ( *( *(file_inode->indirect_block_ptr) + ((block_num-NUM_DIRECT_PTRS-NUM_PTRS_PER_BLK)/NUM_PTRS_PER_BLK) ) + ((block_num-NUM_DIRECT_PTRS-NUM_PTRS_PER_BLK)%NUM_PTRS_PER_BLK) )->data[num_bytes_written%UFS_BLOCK_SIZE] = *(buf + num_bytes_written);
//         }
//         else
//         {   
//             // should never reach here
//             printf("num_bytes_to_write too large\n");
//             return -1;
//         }
        
//         num_bytes_written++;
//     }

//     return num_bytes_written;
// }

// HELPER SUBROUTINES

static inode_t *
get_file_inode(char * path, ufs_dirblock_t * dir_blk)
{
    inode_t * inode_ptr;
	
	if (!path)
		return NULL;

    int i;
    for (i = 0; i < UFS_MAX_FILE_IN_DIR; i++)
    {
        if (str_is_prefix(path, dir_blk->entries[i].filename))
        {
            inode_ptr = __inode_array + dir_blk->entries[i].inode_num;

            // the found prefix could be an incomplete path to a dir block
            if (inode_ptr->type == DIR)
            {
                path = strtok(path, UFS_DIR_DELIM);
                return get_file_inode(path, inode_ptr->direct_block_ptrs[0]);
            }
            
            // or the file itself
            else return inode_ptr;
        }
    }

    return NULL;
}


static void
set_blk_bitmap(int blk_index, inode_status_t mark)
{
    if (blk_index < 0 ||
        blk_index > (UFS_NUM_BITMAP_BLOCKS * UFS_BLOCK_SIZE) / sizeof(uint8_t))
        return;
    
    uint8_t val = __blk_bitmap[blk_index / (sizeof(uint8_t) * 8)];
    uint8_t pos = __blk_bitmap[blk_index % (sizeof(uint8_t) * 8)];

    if (mark == FREE)
        val &= ~(1 << pos);
    else // mark OCCUPIED
        val |= (1 << pos);

    __blk_bitmap[blk_index / (sizeof(uint8_t) * 8)] = val;
}


static bool
is_blk_free(int blk_index)
{
    if (blk_index < 0 || blk_index > UFS_NUM_MAX_INODES)
        return false;
    
    uint8_t val = __blk_bitmap[blk_index / (sizeof(uint8_t) * 8)];
    uint8_t pos = __blk_bitmap[blk_index % (sizeof(uint8_t) * 8)];

    return ((val >> pos) & 1);
}
