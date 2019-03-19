#include <kmalloc.h>
#include <ramdisk.h>
#include <sched.h>
#include <sys/kthread.h>
#include <sys/mutex.h>
#include <sys/stdlib.h>
#include <sys/ufs.h>
#include <sys/vfs.h>

// global fs checkpointers
static ufs_superblock_t *__superblk;
static inode_t *__inode_array;
static uint8_t *__blk_bitmap;
static ufs_dirblock_t *__root_blk;

// lock for the globl pointers
static kthread_mutex_t __fs_head_lock;

// bitmap ops
static void set_blk_bitmap(int, block_status_t);
static bool get_blk_bitmap(int);

// helpers
static inode_t *get_file_inode(char *, ufs_dirblock_t *);
static inode_t *get_parent_dir_inode(char *, ufs_dirblock_t *);

// low level file block manipulators
static ufs_datablock_t *alloc_block(void);
static void dealloc_block(ufs_datablock_t *);
// static void __expand_file(inode_t *, size_t);
// static void __shrink_file(inode_t *, size_t);

extern task_struct_t *__current_task;


/*
 * initialization routine for the ramdisk.
 * sets up a 2MB ramdisk starting at the input base address.
 * @param fs_base_addr: base address of the ramdisk. (Caller allocated)
 **/
void init_rdisk(void *fs_base_addr)
{
	/* 0: initialize the whole 2 MB region to null */
	memset(fs_base_addr, 0, UFS_DISK_SIZE);

	/* 1: INIT SUPERBLOCK */
	__superblk					= (ufs_superblock_t *)fs_base_addr;
	__superblk->magic			= UFS_HEADER_MAGIC;
	__superblk->block_size		= UFS_BLOCK_SIZE;
	__superblk->num_blocks		= UFS_NUM_MAX_BLOCKS;
	__superblk->num_free_blocks = UFS_NUM_MAX_BLOCKS - 1;
	__superblk->num_inodes		= UFS_NUM_MAX_INODES;
	__superblk->num_free_inodes = UFS_NUM_MAX_INODES - 1;

	/* 2: INIT INODE ARRAY */
	__inode_array = (inode_t *)fs_base_addr + UFS_BLOCK_SIZE;

	/* 3: INIT INODE BITMAP */
	__blk_bitmap
		= (uint8_t *)__inode_array + (UFS_NUM_MAX_INODES * sizeof(inode_t));
	set_blk_bitmap(0, BLK_STATUS_OCCUPIED);

	/* 4: INIT ROOT DIRECTORY BLOCK AND INODE */
	__root_blk = (ufs_dirblock_t *)__blk_bitmap
		+ (UFS_BLOCK_SIZE * UFS_NUM_BITMAP_BLOCKS);
	__inode_array[0].type		  = INODE_DIR;
	__inode_array[0].dirblock_ptr = __root_blk;

	__superblk->inode_array = (inode_t *)__inode_array;
	__superblk->blk_bitmap  = (uint8_t *)__blk_bitmap;
	__superblk->root_blk	= (ufs_dirblock_t *)__root_blk;

	kthread_mutex_init(&__fs_head_lock);
}


/*
 * creates a new REGULAR file at the input path
 * @return 0 if successful. Error codes otherwise
 **/
int rd_create(char *path)
{
	inode_t *file_inode;
	inode_t *parent_dir_inode;
	ufs_dirent_t *dirent;
	int i;
	int inode_counter;

	if (!path)
		return EBADPATH;

	parent_dir_inode = get_parent_dir_inode(path, __root_blk);

	// check if valid path
	if (!parent_dir_inode)
		return EBADPATH;

	// check if parent dir has space left for a file
	dirent = NULL;
	for (i = 0; i < UFS_MAX_FILE_IN_DIR; i++) {
		if (parent_dir_inode->dirblock_ptr->entries[i].filename[0] == NULL) {
			dirent = &parent_dir_inode->dirblock_ptr->entries[i];
			break;
		}
	}

	// dir is full, cannot create anymore files
	if (!dirent)
		return EBOUNDS;

	// add new file's inode to the inode array
	kthread_mutex_lock(&__fs_head_lock);

	// search for the inode
	inode_counter = 0;
	file_inode	= &__inode_array[0];
	while (inode_counter < UFS_NUM_MAX_INODES) {
		if (file_inode->type == INODE_EMPTY)
			break;

		inode_counter++;
		file_inode++;
	}

	if (inode_counter == UFS_NUM_MAX_INODES) {
		kthread_mutex_unlock(&__fs_head_lock);
		return EBOUNDS;
	}

	// set inode metadata
	file_inode->type = INODE_REG;
	file_inode->size = 0;
	kthread_mutex_unlock(&__fs_head_lock);

	// set dirent data
	// copy file namy by isolating it from the delimiter
	path += strlen(path);
	while (*path != UFS_DIR_DELIM_CHAR) {
		path--;
	}
	path++;
	memcpy(dirent->filename, path, strlen(path));
	dirent->inode_num = inode_counter;

	return 0;
}


/*
 * creates a new DIRECTORY file at the input path
 * @return 0 if successful. Error codes otherwise
 **/
int rd_mkdir(char *path)
{
	inode_t *dir_inode;
	inode_t *parent_dir_inode;
	ufs_dirent_t *dirent;
	int i;
	int inode_counter;

	if (!path)
		return EBADPATH;

	parent_dir_inode = get_parent_dir_inode(path, __root_blk);

	// check if valid path
	if (!parent_dir_inode)
		return EBADPATH;

	// check if parent dir has space left for a file
	dirent = NULL;
	for (i = 0; i < UFS_MAX_FILE_IN_DIR; i++) {
		if (parent_dir_inode->dirblock_ptr->entries[i].filename[0] == NULL) {
			dirent = parent_dir_inode->dirblock_ptr->entries + i;
			break;
		}
	}

	// dir is full, cannot create anymore files
	if (!dirent)
		return EBOUNDS;

	// add new file's inode to the inode array
	kthread_mutex_lock(&__fs_head_lock);

	// search for the inode
	inode_counter = 0;
	dir_inode	 = &__inode_array[0];
	while (inode_counter < UFS_NUM_MAX_INODES) {
		if (dir_inode->type == INODE_EMPTY)
			break;

		inode_counter++;
		dir_inode++;
	}

	if (inode_counter == UFS_NUM_MAX_INODES) {
		kthread_mutex_unlock(&__fs_head_lock);
		return EBOUNDS;
	}

	// set inode metadata
	dir_inode->type = INODE_DIR;
	dir_inode->size = 0;
	kthread_mutex_unlock(&__fs_head_lock);

	dir_inode->dirblock_ptr = (ufs_dirblock_t *)alloc_block();

	// set dirent data
	// copy file namy by isolating it from the delimiter
	path += strlen(path);
	while (*path != UFS_DIR_DELIM_CHAR) {
		path--;
	}
	path++;
	memcpy(dirent->filename, path, strlen(path));
	dirent->inode_num = inode_counter;

	return 0;
}


/*
 * open the file at the input path
 * @return fd if successful. Error codes otherwise
 **/
int rd_open(char *path)
{
	// see if this proc can open any more files
	// int fd = get_avail_fd();
	int fd = -1;
	int i;
	for (i = 0; i < NUM_MAX_FD; i++) {
		if (!(__current_task->fd_table[i])) {
			fd = i;
			break;
		}
	}

	if (fd == -1)
		return EMAXF;

	// check path validity
	inode_t *file_inode = get_file_inode(path, __root_blk);

	// file path was invalid
	if (!file_inode)
		return EBADPATH;

	// create control block file object and set metadata
	FILE *file_obj		= kmalloc(sizeof(FILE));
	file_obj->fd		= fd;
	file_obj->inode_ptr = file_inode;
	file_obj->path		= (char *)kmalloc(strlen(path));
	strcpy(path, file_obj->path);
	file_obj->seek_head = 0;

	// add the file obj in the control block at the opended fd
	__current_task->fd_table[fd] = file_obj;
	return fd;
}


/*
 * close the file pointed to by the fd.
 * @return 0 if successful. error codes otherwise
 **/
int rd_close(int fd)
{
	if (fd < 0 || fd > NUM_MAX_FD)
		return EBADF;

	FILE *file_obj = __current_task->fd_table[fd];
	if (!file_obj)
		return EBADF;

	kfree(file_obj->path);
	kfree(file_obj);
	__current_task->fd_table[fd] = NULL;
	return 0;
}


/*
 * move seek head according to whence mode and input offset
 * @return 0 if successful. error codes otherwise
 **/
int rd_lseek(int fd, int offset, int whence)
{
	if (fd < 0 || fd > NUM_MAX_FD)
		return EBADF;

	FILE *file_obj = __current_task->fd_table[fd];
	if (!file_obj)
		return EBADF;

	// cannot seek if not a regular file
	if (file_obj->inode_ptr->type != INODE_REG)
		return ENOREG;

	switch (whence) {
		case SEEK_SET: {
			if (offset < 0 || offset > UFS_MAX_FILESIZE)
				return EBOUNDS;
			else
				file_obj->seek_head = offset;
		} break;
		case SEEK_CUR: {
			if (file_obj->seek_head + offset < 0
				|| file_obj->seek_head + offset > UFS_MAX_FILESIZE)
				return EBOUNDS;
			else
				file_obj->seek_head += offset;
		} break;
		case SEEK_END: {
			file_obj->seek_head = file_obj->inode_ptr->size;
		} break;
		default: {
			return EINVAL;
		}
	}

	return 0;
}


/*
 * read from file num_bytes bytes pointed to by the given fd to buf
 * @return 0 if successful. Error codes otherwise
 **/
int rd_read(int fd, char *buf, int num_bytes)
{
	FILE *file_obj;
	inode_t *file_inode;

	if (fd < 0 || fd > NUM_MAX_FD)
		return EBADF;

	file_obj   = __current_task->fd_table[fd];
	file_inode = file_obj->inode_ptr;

	// check if valid fd
	if (!file_obj)
		return EBADF;

	// bad buffer
	if (!buf)
		return EBADBUF;

	// file overread
	if (num_bytes < 0 || num_bytes > file_inode->size - file_obj->seek_head)
		return EBOUNDS;

	// not a regular readable file
	if (file_obj->inode_ptr->type != INODE_REG)
		return ENOREG;

	int read_counter = 0;

	// first we set up the scanner read head blk pointers and indexes
	ufs_datablock_t ***double_blk_ptr = file_inode->double_indirect_block_ptr;
	ufs_datablock_t **single_blk_ptr  = file_inode->indirect_block_ptr;
	ufs_datablock_t **direct_blk_ptr  = file_inode->direct_block_ptrs;
	ufs_datablock_t *blk_ptr;
	int double_blk_idx;
	int single_blk_idx;
	int blk_num	= file_obj->seek_head / UFS_BLOCK_SIZE;
	int blk_offset = file_obj->seek_head % UFS_BLOCK_SIZE;

	// SETUP PONITERS AND INDEXES FOR READING
	// reading starting from direct mapped region
	if (blk_num < UFS_NUM_DIRECT_PTRS) {
		// set initial indexes
		double_blk_idx = 0;
		single_blk_idx = 0;

		// set initial pointers
		blk_ptr = file_inode->direct_block_ptrs[blk_num];
	}

	// reading starting from singly differed region
	else if (blk_num < UFS_NUM_DIRECT_PTRS + UFS_NUM_PTRS_PER_BLK) {
		// set initial indexes
		double_blk_idx = 0;
		single_blk_idx = (blk_num - UFS_NUM_DIRECT_PTRS) % UFS_NUM_PTRS_PER_BLK;

		// set initial pointers
		blk_ptr = single_blk_ptr[single_blk_idx];
	}

	// rading starting from doubly deffered region
	else {
		// set initial indexes
		double_blk_idx = (blk_num - UFS_NUM_DIRECT_PTRS - UFS_NUM_PTRS_PER_BLK)
			/ UFS_NUM_PTRS_PER_BLK;
		single_blk_idx = (blk_num - UFS_NUM_DIRECT_PTRS - UFS_NUM_PTRS_PER_BLK)
			% UFS_NUM_PTRS_PER_BLK;

		// set initial pointers
		single_blk_ptr = double_blk_ptr[double_blk_idx];
		blk_ptr		   = single_blk_ptr[single_blk_idx];
	}

	// START READING
	goto start_reading;
	while (double_blk_idx < UFS_NUM_PTRS_PER_BLK) {
		while (single_blk_idx < UFS_NUM_PTRS_PER_BLK) {
		start_reading:
			while (blk_offset < UFS_BLOCK_SIZE) {
				*buf++ = blk_ptr->data[blk_offset++];
				if (++read_counter == num_bytes)
					break;
			}

			if (read_counter == num_bytes)
				break;

			blk_offset = 0;
			blk_num++;

			// if we're still reading from blocks direct pointer, blk_num is the
			// index
			if (blk_num < UFS_NUM_DIRECT_PTRS) {
				blk_ptr = direct_blk_ptr[blk_num];
			} else {
				// do not increase idx if blk_num==8
				if (blk_num > UFS_NUM_DIRECT_PTRS)
					single_blk_idx++;

				blk_ptr = single_blk_ptr[single_blk_idx];
			}
		}

		if (read_counter == num_bytes)
			break;

		// since it will only be met when we first start reading from the doubly
		// deffered region
		if (blk_num > UFS_NUM_DIRECT_PTRS + UFS_NUM_PTRS_PER_BLK)
			double_blk_idx++;
		single_blk_ptr = double_blk_ptr[double_blk_idx];

		single_blk_idx = 0;
		blk_ptr		   = single_blk_ptr[single_blk_idx];
	}

	file_obj->seek_head += num_bytes;
	return 0;
}


/*
 * write to file num_bytes bytes pointed to by the given fd from buf
 * @return 0 if successful. Error codes otherwise
 **/
int rd_write(int fd, char *buf, int num_bytes)
{
	FILE *file_obj;
	inode_t *file_inode;

	if (fd < 0 || fd > NUM_MAX_FD)
		return EBADF;

	file_obj   = __current_task->fd_table[fd];
	file_inode = file_obj->inode_ptr;

	// check if valid fd
	if (!file_obj)
		return EBADF;

	// bad buffer
	if (!buf)
		return EBADBUF;

	// file overwrite
	if (num_bytes < 0 || num_bytes + file_obj->seek_head > UFS_MAX_FILESIZE)
		return EBOUNDS;

	// not a regular readable file
	if (file_obj->inode_ptr->type != INODE_REG)
		return ENOREG;

	// Preprocess the pointers for the start
	int write_counter = 0;
	// if (file_obj->seek_head > file_inode->size)
	//     __expand_file(file_inode, file_obj->seek_head - file_inode->size);

	// first we set up the scanner read head blk pointers and indexes
	ufs_datablock_t ***double_blk_ptr = file_inode->double_indirect_block_ptr;
	ufs_datablock_t **single_blk_ptr  = file_inode->indirect_block_ptr;
	ufs_datablock_t **direct_blk_ptr  = file_inode->direct_block_ptrs;
	ufs_datablock_t *blk_ptr;
	int double_blk_idx;
	int single_blk_idx;
	// fixme: if seek head is mire than size, write 0's from size to seek head
	int blk_num			 = file_obj->seek_head / UFS_BLOCK_SIZE;
	int blk_offset		 = file_obj->seek_head % UFS_BLOCK_SIZE;
	int actual_num_bytes = file_obj->seek_head + num_bytes - file_inode->size;

	// SETUP PONITERS AND INDEXES FOR READING
	// writing starting in direct mapped region
	if (blk_num < UFS_NUM_DIRECT_PTRS) {
		// set initial indexes
		double_blk_idx = 0;
		single_blk_idx = 0;

		// set initial pointers
		if (!file_inode->direct_block_ptrs[blk_num])
			file_inode->direct_block_ptrs[blk_num]
				= (ufs_datablock_t *)alloc_block();
		blk_ptr = file_inode
					  ->direct_block_ptrs[file_obj->seek_head % UFS_BLOCK_SIZE];
	}

	// writing starting in singly differed region
	else if (blk_num < UFS_NUM_DIRECT_PTRS + UFS_NUM_PTRS_PER_BLK) {
		// set initial indexes
		double_blk_idx = 0;
		single_blk_idx = (blk_num - UFS_NUM_DIRECT_PTRS) % UFS_NUM_PTRS_PER_BLK;

		// set initial pointers
		if (!single_blk_ptr) {
			single_blk_ptr				   = (ufs_datablock_t **)alloc_block();
			file_inode->indirect_block_ptr = single_blk_ptr;
		}

		if (!single_blk_ptr[single_blk_idx])
			single_blk_ptr[single_blk_idx] = (ufs_datablock_t *)alloc_block();
		blk_ptr = single_blk_ptr[single_blk_idx];
	}

	// writing starting in doubly deffered region
	else {
		// set initial indexes
		double_blk_idx = (blk_num - UFS_NUM_DIRECT_PTRS - UFS_NUM_PTRS_PER_BLK)
			/ UFS_NUM_PTRS_PER_BLK;
		single_blk_idx = (blk_num - UFS_NUM_DIRECT_PTRS - UFS_NUM_PTRS_PER_BLK)
			% UFS_NUM_PTRS_PER_BLK;

		// set initial pointers
		if (!double_blk_ptr) {
			double_blk_ptr = (ufs_datablock_t ***)alloc_block();
			file_inode->double_indirect_block_ptr = double_blk_ptr;
		}

		if (!double_blk_ptr[double_blk_idx])
			double_blk_ptr[double_blk_idx] = (ufs_datablock_t **)alloc_block();
		single_blk_ptr = double_blk_ptr[double_blk_idx];

		if (!single_blk_ptr[single_blk_idx])
			single_blk_ptr[single_blk_idx] = (ufs_datablock_t *)alloc_block();
		blk_ptr = single_blk_ptr[single_blk_idx];
	}

	// START WRITING
	goto start_writing;
	while (double_blk_idx < UFS_NUM_PTRS_PER_BLK) {
		while (single_blk_idx < UFS_NUM_PTRS_PER_BLK) {
		start_writing:
			while (blk_offset < UFS_BLOCK_SIZE) {
				// we could be writing nulls in the seek head 'hole'
				if (write_counter < file_obj->seek_head - file_inode->size)
					blk_ptr->data[blk_offset++] = 0;
				else
					blk_ptr->data[blk_offset++] = *buf++;

				if (++write_counter == actual_num_bytes)
					break;
			}

			if (write_counter == actual_num_bytes)
				break;

			blk_offset = 0;
			blk_num++;

			// if reading from the deffered blocks (single or double) in the
			// inode
			if (blk_num > UFS_NUM_DIRECT_PTRS) {
				single_blk_idx++;
				if (!single_blk_ptr)
					single_blk_ptr = (ufs_datablock_t **)alloc_block();

				if (!single_blk_ptr[single_blk_idx])
					single_blk_ptr[single_blk_idx]
						= (ufs_datablock_t *)alloc_block();
				blk_ptr = single_blk_ptr[single_blk_idx];
			}
			// otherwise reading form direct block pointers of inode
			else {
				if (!direct_blk_ptr[blk_num])
					direct_blk_ptr[blk_num] = (ufs_datablock_t *)alloc_block();
				blk_ptr = direct_blk_ptr[blk_num];
			}
		}

		if (write_counter == actual_num_bytes)
			break;

		// if statement of the edge case of the single indirect ptr in inode
		if (blk_num > UFS_NUM_DIRECT_PTRS + UFS_NUM_PTRS_PER_BLK)
			double_blk_idx++;

		if (!double_blk_ptr)
			double_blk_ptr = (ufs_datablock_t ***)alloc_block();

		if (!double_blk_ptr[double_blk_idx])
			double_blk_ptr[double_blk_idx] = (ufs_datablock_t **)alloc_block();
		single_blk_ptr = double_blk_ptr[double_blk_idx];

		// allocate new data block within singly deffered block if necessary
		single_blk_idx = 0;
		if (!single_blk_ptr[single_blk_idx])
			single_blk_ptr[single_blk_idx] = (ufs_datablock_t *)alloc_block();
		blk_ptr = single_blk_ptr[single_blk_idx];
	}


	// update size of file, if necessary (if the last byte written was after the
	// old seek head)
	if (file_inode->size - file_obj->seek_head > num_bytes) {
		file_inode->size = file_inode->size - file_obj->seek_head + num_bytes;
	}

	// modify metadata accordingly and remove any trailing blocks that are not
	// longer needed
	file_obj->seek_head += num_bytes;
	// if (file_obj->seek_head < file_inode->size)
	//     __shrink_file(file_inode, file_inode->size - file_obj->seek_head);

	kthread_mutex_lock(&__fs_head_lock);
	file_inode->size = file_obj->seek_head;
	kthread_mutex_unlock(&__fs_head_lock);
	return 0;
}


/*
 * copies a dirent to the calling process for the dir opened at the input fd
 * keeps track of the last read dir with the FILE->dir_pos
 **/
int rd_readdir(int fd, char *buf)
{
	ufs_dirblock_t *dir_ptr;
	FILE *file_obj;

	if (fd < 0 || fd > NUM_MAX_FD)
		return EBADF;

	file_obj = __current_task->fd_table[fd];
	if (!file_obj)
		return EBADF;

	// fd does not point to a dir
	if (file_obj->inode_ptr->type != INODE_DIR)
		return ENODIR;

	dir_ptr = file_obj->inode_ptr->dirblock_ptr;

	// we want to ignore empty entires
	while (dir_ptr->entries[file_obj->dir_pos].filename == NULL
		&& file_obj->dir_pos < UFS_MAX_FILE_IN_DIR)
		file_obj->dir_pos++;

	if (file_obj->dir_pos == UFS_MAX_FILE_IN_DIR) {
		file_obj->dir_pos = 0;
		return 1;
	}

	memcpy(buf, (void *)&dir_ptr->entries[file_obj->dir_pos++],
		sizeof(ufs_dirent_t));

	while (dir_ptr->entries[file_obj->dir_pos].filename == NULL
		&& file_obj->dir_pos < UFS_MAX_FILE_IN_DIR)
		file_obj->dir_pos++;

	// last dirent in this dir
	if (file_obj->dir_pos == UFS_MAX_FILE_IN_DIR) {
		file_obj->dir_pos = 0;
		return 1;
	}

	// not the last dirent in this dir
	return 0;
}


/*
 * deletes a file by unlinking all block pointers from the inode
 * and then deletes the inode of the file itself
 * @return 0 if successful. error codes otherwise
 **/
int rd_unlink(char *path)
{
	int i;
	inode_t *file_inode;
	inode_t *parent_dir_inode;

	/* ERROR CHECKS */
	// check if path is valid
	if (!path)
		return EBADPATH;

	file_inode		 = get_file_inode(path, __root_blk);
	parent_dir_inode = get_parent_dir_inode(path, __root_blk);

	if (!file_inode)
		return EBADPATH;

	// check if file is open
	for (i = 0; i < NUM_MAX_FD; i++) {
		if (__current_task->fd_table[i]
			&& __current_task->fd_table[i]->inode_ptr == file_inode)
			return EINVAL;
	}

	// check if inode file is root block or if the dir in not empty
	if (file_inode->type == INODE_DIR) {
		if (file_inode == &__inode_array[0])
			return EINVAL;

		for (i = 0; i < UFS_MAX_FILE_IN_DIR; i++)
			if (file_inode->dirblock_ptr->entries[i].filename[0])
				return EINVAL;
	}

	/* UNLINK DATA */
	// unlink unlink datablocks
	// first we set up the scanner read head blk pointers and indexes
	ufs_datablock_t ***double_blk_ptr = file_inode->double_indirect_block_ptr;
	ufs_datablock_t **single_blk_ptr  = file_inode->indirect_block_ptr;
	ufs_datablock_t **direct_blk_ptr  = file_inode->direct_block_ptrs;
	// ufs_datablock_t   *blk_ptr;
	int double_blk_idx   = 0;
	int single_blk_idx   = 0;
	int direct_blk_idx   = 0;
	int num_blks_in_file = file_inode->size / UFS_BLOCK_SIZE;
	// round up number of blocks
	if (file_inode->size % UFS_BLOCK_SIZE)
		num_blks_in_file++;

	// start deallocation

	if (num_blks_in_file != 0) {
		goto start_unlink;
		while (double_blk_idx < UFS_NUM_PTRS_PER_BLK) {
			while (single_blk_idx < UFS_NUM_PTRS_PER_BLK) {
			start_unlink:
				if (direct_blk_idx < UFS_NUM_DIRECT_PTRS) {
					dealloc_block(direct_blk_ptr[direct_blk_idx]);
				} else {
					// do not increase idx if direct_blk_idx == 8
					if (direct_blk_idx > UFS_NUM_DIRECT_PTRS)
						single_blk_idx++;

					dealloc_block(single_blk_ptr[single_blk_idx]);
				}

				if (++direct_blk_idx == num_blks_in_file)
					break;
			}

			// deallocate the sigle blk pointer, weather from a double region or
			// direct inode mapped
			if (single_blk_ptr)
				dealloc_block(*single_blk_ptr);

			if (direct_blk_idx == num_blks_in_file)
				break;

			// since it will only be met when we first start reading from the
			// doubly deffered region
			if (direct_blk_idx > UFS_NUM_DIRECT_PTRS + UFS_NUM_PTRS_PER_BLK)
				double_blk_idx++;

			single_blk_ptr = double_blk_ptr[double_blk_idx];
		}

		// deallocate trailing double block pointer
		if (double_blk_ptr)
			dealloc_block((ufs_datablock_t *)*double_blk_ptr);
	}

	/* UNLINK INODE_DIR AND INODE */
	// remove the dirent of this file from parent dirblock
	for (i = 0; i < UFS_MAX_FILE_IN_DIR; i++) {
		if (parent_dir_inode->dirblock_ptr->entries[i].inode_num
			== ((uint16_t)((char *)file_inode - (char *)__inode_array)
				   / sizeof(inode_t))) {
			memset(&(parent_dir_inode->dirblock_ptr->entries[i]), 0,
				sizeof(ufs_dirent_t));
			break;
		}
	}

	/* BLK_STATUS_FREE INODE */
	kthread_mutex_lock(&__fs_head_lock);
	memset(file_inode, 0, sizeof(inode_t));
	kthread_mutex_unlock(&__fs_head_lock);
	return 0;
}


//
// HELPER SUBROUTINES
//


/*
 * get an unallocated datablock from the disk
 * @return pointer to block if successful, NULL otherwise
 **/
static ufs_datablock_t *alloc_block(void)
{
	int i;
	kthread_mutex_lock(&__fs_head_lock);

	// scan the bitmap from start to finish looking for a block that is free
	for (i = 0; i < 8 * UFS_NUM_BITMAP_BLOCKS * UFS_BLOCK_SIZE; i++) {
		if (!get_blk_bitmap(i)) {
			set_blk_bitmap(i, BLK_STATUS_OCCUPIED);
			kthread_mutex_unlock(&__fs_head_lock);
			return (ufs_datablock_t *)__root_blk + i;
		}
	}

	kthread_mutex_unlock(&__fs_head_lock);
	return NULL;
}


/*
 * unallocates a block from the fs
 **/
static void dealloc_block(ufs_datablock_t *blk_ptr)
{
	int blk_num;
	kthread_mutex_lock(&__fs_head_lock);
	blk_num = ((char *)blk_ptr - (char *)__root_blk) / sizeof(UFS_BLOCK_SIZE);
	set_blk_bitmap(blk_num, BLK_STATUS_FREE);
	kthread_mutex_unlock(&__fs_head_lock);
}


/*
 * get the inode of a dir or a reg file given the path to it
 * @return pointer of the inode if path is valid, NULL otherwise
 **/
static inode_t *get_file_inode(char *path, ufs_dirblock_t *dir_blk)
{
	inode_t *inode_ptr;
	int i;

	if (!path)
		return NULL;

	// if the input path is "/", return root block inode
	if (strcmp(path, UFS_DIR_DELIM_STR))
		return &__inode_array[0];

	if (*path == UFS_DIR_DELIM_CHAR)
		path++;

	for (i = 0; i < UFS_MAX_FILE_IN_DIR; i++) {
		if (str_is_prefix(path, dir_blk->entries[i].filename)) {
			path = strtok(path, UFS_DIR_DELIM_STR);
			// TODO: the prefix match still is not enough for a full match,
			// two paths could hold the same prefix at different depths in the
			// file system we have only found the file we are looking for if the
			// path no longer has any path delimitors left

			inode_ptr = &__inode_array[dir_blk->entries[i].inode_num];

			// the found prefix could be an incomplete path to a dir block
			if (inode_ptr->type == INODE_DIR)
				return get_file_inode(path, inode_ptr->dirblock_ptr);
			// or the file itself
			else
				return inode_ptr;
		}
	}
	return NULL;
}


/*
 * get the inode of the parent directory of the provided path
 * @return pointer of the inode to the INODE_DIR file if found, NULL otherwise
 **/
static inode_t *get_parent_dir_inode(char *path, ufs_dirblock_t *dir_blk)
{
	inode_t *inode_ptr;
	char splice_char;
	size_t pathlen;

	if (!path)
		return NULL;

	// every path must start at root
	if (path[0] != UFS_DIR_DELIM_CHAR)
		return NULL;

	// user can either input path as "root/dir_to_find/" or "/root/dir_to_find"
	pathlen = strlen(path);
	if (path[pathlen - 1] == UFS_DIR_DELIM_CHAR)
		pathlen--;

	// now seperate the lowest level name from its parent path
	while (path[pathlen - 1] != UFS_DIR_DELIM_CHAR)
		pathlen--;

	// but the UFS_DIR_DELIM_CHAR could point to the "/" or some other directory
	// dir this changes where we insert the splicing null
	if (path[pathlen - 1] == UFS_DIR_DELIM_CHAR && pathlen > 1)
		pathlen--;

	// splice that seperator out and feed the parent path to the get file inode
	// function
	splice_char   = path[pathlen];
	path[pathlen] = NULL;

	inode_ptr = get_file_inode(path, dir_blk);

	// restore path and return inode
	path[pathlen] = splice_char;
	return inode_ptr;
}


/*
 * setter: bitmap helper that sets a the bit of the block index to the input
 *mark
 **/
static void set_blk_bitmap(int blk_index, block_status_t mark)
{
	uint8_t val, pos;

	if (blk_index < 0
		|| blk_index
			> (UFS_NUM_BITMAP_BLOCKS * UFS_BLOCK_SIZE) / sizeof(uint8_t))
		return;

	val = __blk_bitmap[blk_index / (sizeof(uint8_t) * 8)];
	pos = blk_index % (sizeof(uint8_t) * 8);

	if (mark == BLK_STATUS_FREE)
		val &= ~(1 << pos);
	else // mark BLK_STATUS_OCCUPIED
		val |= (1 << pos);

	__blk_bitmap[blk_index / (sizeof(uint8_t) * 8)] = val;
}


/*
 * getter: bitmap helper that gets a the value of the mark of the input block
 *index
 **/
static bool get_blk_bitmap(int blk_index)
{
	uint8_t val, pos;

	if (blk_index < 0 || blk_index > UFS_NUM_MAX_INODES)
		return false;

	val = __blk_bitmap[blk_index / (sizeof(uint8_t) * 8)];
	pos = blk_index % (sizeof(uint8_t) * 8);

	return ((val >> pos) & 1);
}
