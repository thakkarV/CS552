#include <kmalloc.h>
#include <multiboot.h>
#include <kstdlib.h>
#include <types.h>
#include <kvideo.h>

// #define DEBUG

#define ALIGNTO(size, boundry) (size + boundry - 1) & ~(boundry - 1)
#define BOUNDRY 16
#define BLK_FRAGEMENT_THRESHOLD 32

/* bytes to be added for each malloc call for the bookeeping BLK_HEADER_SIZE */
#define BLK_HEADER_SIZE sizeof(block_header_t)


/* internal methods */
static block_header_t * __get_block_header(void *);
static block_header_t * __splice_block_out(block_header_t *, block_header_t *);
static block_header_t * __splice_block_in(block_header_t *, size_t);


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
	__mem_store_base = store_addr;
	__mem_store_end = (void *) (((char *) store_addr) + store_size);
	__mem_store_size = store_size;

	/* make initial single entry for the free store */
	__store_global_head = (block_header_t *) __mem_store_base;
	__store_global_head->prev = NULL;
	__store_global_head->next = NULL;
	__store_global_head->size = __mem_store_size - BLK_HEADER_SIZE;
	__store_global_head->is_free = true;

	#ifdef DEBUG
		printf("Global store head = 0x%x\n", __store_global_head);
		printf("Global store size = 0x%x\n", __store_global_head->size);
	#endif
}


void *
kmalloc(size_t size)
{
	if (size <= 0)
		return NULL;

	#ifdef DEBUG
		printf("alloc size = 0x%x | ", size);
	#endif
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
	while (current && (current->size < alloc_size) && !(current->is_free))
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
	#ifdef DEBUG
		printf("Retruning data pointer = 0x%x", current + 1);
	#endif
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
	block_header_t * header;

	if (!buffer || !__store_global_head)
		return;

	header = __get_block_header(buffer);

	#ifdef DEBUG
		printf("kfree called on buffer 0x%x\n", buffer);
		printf("kfree header 0x%x, blk size = 0x%x\n", header, header->size);
	#endif

	if (!header)
		return;

	
	header->is_free = true;

	// now splice this block out if the surrounding blocks are also free
	if (header->next)
	{
		if (header->next->is_free)
		{
			// header = __splice_block_out(header, header->next);
			header->is_free = true;
		}
	}
	
	if (header->prev)
	{
		if (header->prev->is_free)
		{
			// header = __splice_block_out(header->prev, header);
			header->is_free = true;
		}
	}
}


static block_header_t *
__get_block_header(void * buffer)
{
	block_header_t * current = __store_global_head;

	block_header_t * lookup_header;
	char * tmp = buffer;
	tmp -= BLK_HEADER_SIZE;
	lookup_header = (block_header_t *) tmp;

	#ifdef DEBUG
		printf("getter called
		 on buffer 0x%x\n", buffer);
		printf("getter lookup header = 0x%x\n", lookup_header);
	#endif

	while (current != NULL)
	{
		if (lookup_header == current)
			return lookup_header;

		current = current->next;
	}
	return NULL;
}


static block_header_t *
__splice_block_out(block_header_t * lo_header, block_header_t * hi_header)
{
	#ifdef DEBUG
		printf("splicing 0x%x and 0x%x together\n", lo_header, hi_header);
	#endif
	// change size
	lo_header->size += hi_header->size + BLK_HEADER_SIZE;

	// splice pointers
	lo_header->next = hi_header->next;
	hi_header->next->prev = lo_header;

	return lo_header;
}


static block_header_t *
__splice_block_in(block_header_t * current, size_t alloc_size)
{
	#ifdef DEBUG
		printf("Current old block pointer = 0x%x with size = 0x%x\n", current, current->size);
	#endif
	/* init new block header if we have the space to do so
	 * notice we have to add the current block header offset
	**/
	if (current-> size > alloc_size + 2 * BLK_HEADER_SIZE)
	{
		block_header_t * new = (block_header_t *) ((char *) current + alloc_size + BLK_HEADER_SIZE);
		new->prev = current;
		new->next = NULL;

		// if this block header is not at the end of the free store,
		// thne we need to shuffle some more pointers
		if (current->next)
		{
			new->next = current->next;
		}
		
		// new size is old - what we have to allocate - new's block header size
		new->size = current->size - alloc_size - BLK_HEADER_SIZE;
		new->is_free = true;

		// now shrink the current one and point next to new
		current->next = new;
		current->size = alloc_size;
	}
	
	#ifdef DEBUG
		printf("New block pointer = 0x%x with size = 0x%x\n", new, new->size);
		printf("Current new block pointer = 0x%x with size = 0x%x\n", current, current->size);
	#endif

	return current;
}
