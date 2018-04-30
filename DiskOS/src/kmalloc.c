#include <sys/kmalloc.h>
#include <sys/multiboot.h>
#include <sys/types.h>
#include <stdlib.h>
#include <kvideo.h>

// #define DEBUG
// #define DEBUGFREE

#define ALIGNTO(size, boundry) (size + boundry - 1) & ~(boundry - 1)
#define BOUNDRY 16
#define BLK_FRAGEMENT_THRESHOLD 32
#define STORE_OFFSET 0xFFFFF

/* bytes to be added for each malloc call for the bookeeping BLK_HEADER_SIZE */
#define BLK_HEADER_SIZE sizeof(block_header_t)


/* internal methods */
static block_header_t * __get_block_header(void *);
static block_header_t * __splice_block_out(block_header_t *, block_header_t *);
static block_header_t * __splice_block_in(block_header_t *, size_t);
static void __debug_print_store(void);


/* LINKED LIST for the free store  */
static block_header_t * __store_global_head = NULL;


/* INIT STATIC VALUES */
static void * __mem_store_base;      /* start of the store */
static void * __mem_store_end;       /* end of the store */
static size_t __mem_store_size;      /* lenght of the store */


void
init_kmalloc(multiboot_info_t * mbi)
{
	/* First extrcact the longest type 1 block from the MBI header */
	void * store_addr = NULL;
	size_t store_size = 0;

	multiboot_memory_map_t *mmap;
	for (mmap = (multiboot_memory_map_t *) mbi->mmap_addr;
		(unsigned long) mmap < mbi->mmap_addr + mbi->mmap_length;
		mmap = (multiboot_memory_map_t *) ((unsigned long) mmap
			 + mmap->size + sizeof (mmap->size)))
	{
		size_t mmap_region_size = 0;
		if (mmap->type == 0x1)
		{
			mmap_region_size = (size_t) (mmap->len & 0xffffffff);
			if (mmap_region_size >= store_size)
			{
				store_addr = (void *) (mmap->addr & 0xffffffff);
				store_size = mmap_region_size;
			}
		}
	}

	/* save free store metadata */
	// this offset business is a totoal hack
	// without this the malloc will start allocating memory 
	// where the kernel code is loaded into by the boot loader
	// need to make this more general than just a hardcoded
	// two meg offset. should work for a long time though. ugh...
	__mem_store_base = store_addr + STORE_OFFSET;
	__mem_store_end = (void *) (((char *) store_addr) + store_size);
	__mem_store_size = store_size - STORE_OFFSET;

	/* make initial single entry for the free store */
	__store_global_head = (block_header_t *) __mem_store_base;
	__store_global_head->prev = NULL;
	__store_global_head->next = NULL;
	__store_global_head->size = __mem_store_size - BLK_HEADER_SIZE;
	__store_global_head->is_free = true;
}


void *
kmalloc(size_t size)
{
	if (size <= 0)
		return NULL;

	// size_t alloc_size = ALIGNTO(size + BLK_HEADER_SIZE, BOUNDRY);
	size_t alloc_size = size;

	block_header_t * current = __store_global_head;

	/* POLICY: First Fit Allocation
	 *
	 *  while current is a valid pointer 
	 * 				AND
	 * current block's size is not enough
	 * 				AND
	 *      current is not free
	**/
	while (!current || (current->size < alloc_size) || !(current->is_free))
		current = current->next;


	/* Ran out of memory to allocate */
	if (current->size < alloc_size)
	{
		printf("kmalloc out of memory in free store. Done goofed!\n");
		return NULL;
	}


	/* MANAGE NEXT BLOCK HEADER
	 * check if we are adding to the end of the free store
	 * and if we have space to allocate antoher region after the current one
	 * if yes, allocate the block header for the new region
	 * otherwise, mark this as the last block
	**/

	// init new block header
	current = __splice_block_in(current, alloc_size);

	/* MARK USED */
	current->is_free = false;

	return current + 1;
}


void *
kcalloc(size_t size)
{
	void * region = kmalloc(size);

	if (region)
		memset(region, 0, size);

	return region;
}


void *
krealloc(void * source, size_t new_size)
{
	/* Sanity Check */
	block_header_t * header = __get_block_header(source);
	if (!header)
		return NULL;

	if (header->size >= new_size)
		return source;

	void * region = kmalloc(new_size);

	/* 
	 * only copy data from the old section and not of the new lenght
	 * this is for security reasons, so that we dont just read the whole memory to the user
	 * then again this is not a level seperated kernel so lol@me
	**/
	if (region)
	{
		block_header_t * old_region = __get_block_header(source);
		memcpy(region, source, old_region->size);
	}

	kfree(source);
	return region;
}


void
kfree(void * buffer)
{
	if (!buffer || !__store_global_head)
		return;

	block_header_t * header = __get_block_header(buffer);

	if (!header)
		return;

	if (header->next && header->next->is_free)
		header = __splice_block_out(header, header->next);

	if (header->prev && header->prev->is_free)
		header = __splice_block_out(header->prev, header);

	header->is_free = true;
}


static block_header_t *
__get_block_header(void * buffer)
{
	block_header_t * current = __store_global_head;
	block_header_t * lookup_header = (block_header_t *) buffer - 1;

	while (true)
	{
		if (current == lookup_header)
			return current;

		if (!current->next)
			return NULL;

		current = current->next;
	}
}


static block_header_t *
__splice_block_out(block_header_t * lo_header, block_header_t * hi_header)
{
	// change size
	lo_header->size += hi_header->size + BLK_HEADER_SIZE;

	// the high header could be the last block in the store, handle it in the edge case
	if (hi_header->next)
	{
		// splice pointers
		lo_header->next = hi_header->next;
		hi_header->next->prev = lo_header;
	}
	else
	{
		lo_header->next = NULL;
	}
	return lo_header;
}


static block_header_t *
__splice_block_in(block_header_t * current, size_t alloc_size)
{
	/* init new block header if we have the space to do so
	 * notice we have to add the current block header offset
	**/
	if (current->size > alloc_size + 2 * BLK_HEADER_SIZE)
	{
		block_header_t * new = (block_header_t *) ((char *) current + alloc_size + BLK_HEADER_SIZE);
		new->prev = current;
		new->next = NULL;

		// if this block header is not at the end of the free store,
		// thne we need to shuffle some more pointers
		if (current->next)
		{
			new->next = current->next;
			current->next->prev = new;
		}
		else
		{
			new->next = NULL;
		}
		
		// new size is old - what we have to allocate - new's block header size
		new->size = current->size - alloc_size - BLK_HEADER_SIZE;
		new->is_free = true;

		// now shrink the current one and point next to new
		current->next = new;
		current->size = alloc_size;
	}
	
	return current;
}


static void
__debug_print_store(void)
{
	block_header_t * current = __store_global_head;

	printf("STORE-> ");
	do
	{
		printf("0x%x (0x%x) %d -> ", current, current->size, current->is_free);
	} while((current = current->next));
	printf("NULL\n");
}
