#include <ramdisk.h>
#include <sys/stdlib.h>
#include <sys/threads.h>
#include <sys/ufs.h>
#include <sys/vfs.h>
#include <sched.h>

// global fs checkpointers
static ufs_superblock_t *__superblk;
static inode_t          *__inode_array;
static uint8_t          *__blk_bitmap;
static ufs_dirblock_t   *__root_blk;
static kthread_mutex_t  *fs_header_lock;

// lock for the globl pointers
static kthread_mutex_t fs_head_lock;

// bitmap ops
static void set_blk_bitmap(int, inode_status_t);
static bool get_blk_bitmap(int);

// helpers
static inode_t * get_file_inode(char *, ufs_dirblock_t *);
static void do_write_preprocess(inode_t *, size_t, int);

// low level file block manipulators
static ufs_datablock_t * alloc_new_block(void);
static void __expand_file(inode_t *, size_t);
static void __shrink_file(inode_t *, size_t);

extern task_struct_t *__current_task;


/* 
 * initialization routine for the ramdisk.
 * sets up a 2MB ramdisk starting at the input base address.
 * Caller responsible for allcation.
**/
void
init_rdisk(void *fs_base_addr)
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

    kthread_mutex_init(&fs_head_lock);
}


/* 
 * open the file at the input path
 * @return fd if successful. Error codes otherwise
**/
int
rd_open(char * path)
{
    // see if this proc can open any more files
    int fd = get_avail_fd();
    if (fd == -1)
        return EMAXF;
    
    // check path validity
    inode_t * file_inode = get_file_inode(path, __root_blk);
	
    // file path was invalid
    if (!file_inode)
        return EBADPATH;

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



/* 
 * close the file pointed to by the fd.
 * @return 0 if successful. -1 otherwise
**/
int
rd_close(int fd)
{
    FILE *file_obj = __current_task->fd_table[fd];
    if (!file_obj)
        return EBADF;
    
    kfree(file_obj->path);
    kfree(file_obj);
    __current_task->fd_table[fd] = NULL;
    return 0;
}


/* 
 * moves the seek head to the absolute input position
**/
int
rd_lseek(int fd, int pos)
{
    FILE *file_obj = __current_task->fd_table[fd];
    if (!file_obj)
        return EBADF;
    
    file_obj->seek_head = pos;
    return 0;
}


/* 
 * read from file num_bytes bytes pointed to by the given fd to buf
 * @return 0 if successful. Error codes otherwise
**/
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
		return EBOUNDS;

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
	// reading starting from direct mapped region
	if (blk_num < UFS_NUM_DIRECT_PTRS)
	{
		// set initial indexes
		double_blk_idx = 0;
		single_blk_idx = 0;

		// set initial pointers
		blk_ptr = file_inode->direct_block_ptrs[file_obj->seek_head % UFS_BLOCK_SIZE];
	}

	// reading starting from singly differed region
	else if (blk_num < UFS_NUM_DIRECT_PTRS + UFS_NUM_PTRS_PER_BLK)
	{
		// set initial indexes
		double_blk_idx = 0;
		single_blk_idx = (blk_num - UFS_NUM_DIRECT_PTRS) % UFS_NUM_PTRS_PER_BLK;
		
		// set initial pointers
		blk_ptr = single_blk_ptr[single_blk_idx];
	}

	// rading starting from doubly deffered region
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

        // if statement of the edge case of the single indirect ptr in inode
		if (blk_num != UFS_NUM_DIRECT_PTRS + UFS_NUM_PTRS_PER_BLK)
            double_blk_idx++;
		single_blk_ptr = double_blk_ptr[double_blk_idx];

		single_blk_idx = 0;
		blk_ptr = single_blk_ptr[single_blk_idx];		
	}

    file_obj->seek_head += num_bytes;    
    return 0;
}


/* 
 * write to file num_bytes bytes pointed to by the given fd from buf
 * @return 0 if successful. Error codes otherwise
**/
int
rd_write(int fd, char * buf, int num_bytes)
{
    FILE * file_obj = __current_task->fd_table[fd];
	inode_t * file_inode = file_obj->inode_ptr;
	
    // check if valid fd
    if (!file_obj)
		return EBADF;
	
	// bad buffer
	if (!buf)
		return EBADBUF;

	// file overwrite
    if (num_bytes < 0 || num_bytes + file_obj->seek_head > UFS_MAX_FILESIZE)
		return EBOUNDS;

    // get file struct
    FILE * file_obj = __current_task->fd_table[fd];
    inode_t * file_inode = file_obj->inode_ptr;
    int write_counter = 0;

    if (file_obj->seek_head > file_inode->size)
        __expand_file(file_inode, file_obj->seek_head - file_inode->size);
    
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
	// writing starting in direct mapped region
	if (blk_num < UFS_NUM_DIRECT_PTRS)
	{
		// set initial indexes
		double_blk_idx = 0;
		single_blk_idx = 0;

		// set initial pointers
        if (!file_inode->direct_block_ptrs[file_obj->seek_head % UFS_BLOCK_SIZE])
            file_inode->direct_block_ptrs[file_obj->seek_head % UFS_BLOCK_SIZE] = alloc_new_block();
		blk_ptr = file_inode->direct_block_ptrs[file_obj->seek_head % UFS_BLOCK_SIZE];
	}

	// writing starting in singly differed region
	else if (blk_num < UFS_NUM_DIRECT_PTRS + UFS_NUM_PTRS_PER_BLK)
	{
		// set initial indexes
		double_blk_idx = 0;
		single_blk_idx = (blk_num - UFS_NUM_DIRECT_PTRS) % UFS_NUM_PTRS_PER_BLK;
		
		// set initial pointers
		blk_ptr = single_blk_ptr[single_blk_idx];
	}

	// writing starting in doubly deffered region
	else
	{
		// set initial indexes
		double_blk_idx = (blk_num - UFS_NUM_DIRECT_PTRS) / UFS_NUM_PTRS_PER_BLK;
		single_blk_idx = (blk_num - UFS_NUM_DIRECT_PTRS) % UFS_NUM_PTRS_PER_BLK;
		
		// set initial pointers
		single_blk_ptr = double_blk_ptr[double_blk_idx];
		blk_ptr = single_blk_ptr[single_blk_idx];
	}

	// START WRITING
	goto start_writing;
	while (double_blk_idx < UFS_NUM_PTRS_PER_BLK)
	{
		while (single_blk_idx < UFS_NUM_PTRS_PER_BLK)
		{
			start_writing:
			while (blk_offset < UFS_BLOCK_SIZE)
			{
                // we could be writing nulls in the seek head 'hole'
                if (write_counter < file_obj->seek_head - file_inode->size)
                    blk_ptr->data[blk_offset++] = 0;                    
                else
                    blk_ptr->data[blk_offset++] = *buf++;

				if (++write_counter == file_obj->seek_head + num_bytes - file_inode->size)
					break;
			}

			if (++write_counter == file_obj->seek_head + num_bytes - file_inode->size)
				break;

			blk_offset = 0;
			blk_num++;
			// handle direct pointer edge case
			if (blk_num < UFS_NUM_DIRECT_PTRS)
			{
                if (!direct_blk_ptr[blk_num])
                    direct_blk_ptr[blk_num] = alloc_new_block();
				blk_ptr = direct_blk_ptr[blk_num];
			}
			else
			{
                single_blk_idx++;
                if (!single_blk_ptr)
                    single_blk_ptr = alloc_new_block();

                if (!single_blk_ptr[single_blk_idx])
                    single_blk_ptr[single_blk_idx] = alloc_new_block();
				blk_ptr = single_blk_ptr[single_blk_idx];
			}
		}

	    if (++write_counter == file_obj->seek_head + num_bytes - file_inode->size)
			break;

        // if statement of the edge case of the single indirect ptr in inode
		if (blk_num != UFS_NUM_DIRECT_PTRS + UFS_NUM_PTRS_PER_BLK)
            double_blk_idx++;
        
        if (!double_blk_ptr)
            double_blk_ptr = alloc_new_block();
		single_blk_ptr = double_blk_ptr[double_blk_idx];

        // allocate new data block within singly deffered block if necessary        
		single_blk_idx = 0;
        if (!single_blk_ptr)
            single_blk_ptr = alloc_new_block();
        
        if (!single_blk_ptr[single_blk_idx])
            single_blk_ptr[single_blk_idx] = alloc_new_block();
		blk_ptr = single_blk_ptr[single_blk_idx];
    }


    // modify metadata accordingly and remove any trailing blocks that are not longer needed
    file_obj->seek_head += num_bytes;    
    if (file_obj->seek_head < file_inode->size)
        __shrink_file(file_inode, file_inode->size - file_obj->seek_head);
    file_inode->size = file_obj->seek_head;
    return 0;
}

//
// HELPER SUBROUTINES
//

/* 
 * EXPAND new sections of the file or SHRINK old sections before writing new data
 * wrting beyond the current file size by moving the seek head ahead using lseek
 * writing to file before the last byte of file by moving seek head behin using lseek
**/ 
static void
do_write_preprocess(inode_t *inode, size_t seek_head, int num_bytes)
{
    // expan file up
    if (seek_head + num_bytes > inode->size)
    {
        __expand_file(inode, (seek_head + num_bytes) - inode->size);
    }

    // shrink file down
    else if (seek_head + num_bytes < inode->size)
    {
        __shrink_file(inode, inode->size - (seek_head + num_bytes));
    }

    // nothing to do if wrting just as many bytes as the offset from EOF
    else return;
}


/* 
 * get an unallocated datablock from the disk
 * @return pointer to block if successful, NULL otherwise
**/ 
static ufs_datablock_t *
alloc_new_block(void)
{
    kthread_mutex_lock(&fs_head_lock);
    
    // scan the bitmap from start to finish looking for a block that is free
    int i;
    for (i = 0; i < 8 * UFS_NUM_BITMAP_BLOCKS * UFS_BLOCK_SIZE; i++)
    {
        if (get_blk_bitmap(i))
        {
            set_blk_bitmap(i, OCCUPIED);
            kthread_mutex_unlock(&fs_head_lock);
            return (ufs_datablock_t *) __root_blk + i;
        }
    }
    kthread_mutex_unlock(&fs_head_lock);
    return NULL;
}


/* 
 * get the inode of a dir or a reg file given the path to it
 * @return pointer of the inode if path is valid, NULL otherwise
**/
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


/* 
 * setter: bitmap helper that sets a the bit of the block index to the input mark
**/
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


/* 
 * getter: bitmap helper that gets a the value of the mark of the input block index
**/
static bool
get_blk_bitmap(int blk_index)
{
    if (blk_index < 0 || blk_index > UFS_NUM_MAX_INODES)
        return false;
    
    uint8_t val = __blk_bitmap[blk_index / (sizeof(uint8_t) * 8)];
    uint8_t pos = __blk_bitmap[blk_index % (sizeof(uint8_t) * 8)];

    return ((val >> pos) & 1);
}
