#include "kmalloc.h"
#include "stdlib.h"
#include "bool.h"

#define ALIGNTO(size, boundry) (size + boundry - 1) & ~(boundry - 1)
#define BOUNDRY 16
#define FRAGEMENT_THRESHOLD 32

/* bytes to be added for each malloc call for the bookeeping overhead */
#define OVERHEAD sizeof(block_header_t);

/* LINKED LIST for the free store  */ 
static const block_header_t * __store_global_head = NULL;

/* INIT STATIC VALUES */ 	
static void * __mem_store_base;      /* start of the store */
static void * __mem_store_end;       /* end of the store */
static size_t __mem_store_size;      /* lenght of the store */


void
init_kmalloc(void * store_addr, size_t store_size)
{
	/* save inputs for later use */
	__mem_store_base = store_addr;
	__mem_store_end = (void *) (((char *) store_addr) + store_size);
	__mem_store_size = store_size;

	/* make initial single entry for the free store */
	__store_global_head = (block_header_t *) list_addr;

	__store_global_head->prev = NULL;
	__store_global_head->next = NULL;
	__store_global_head->size = store_size - OVERHEAD;
	__store_global_head->is_free = true;
}


void *
kmalloc(size_t size)
{
	size_t alloc_size = ALIGNTO(size + OVERHEAD, BOUNDRY);

	block_header_t *current = __store_global_head;
	while(current && (currrent->size >= alloc_size) && (current->is_free))
		current = current->next;
	

	/* Ran out of memory to allocate */
	if (current->size < alloc_size)
		return NULL;


	/* MANAGE NEXT
	 * check if we are adding to the end of the free store
	 * and if we have space to allocate antoher region after the current one
	 * if yes, allocate the block header for the new region
	 * otherwise, mark this as the last block
	**/
	if (current->next)
	{
		/* MANAGE FRAGMENTATION AND SIZE OF CURRENT
		 * chop this block off into two if it has at least 32 bytes of space wasted
		 * this requires initializing a new block header in the middle of the current block
		 * and dealing with resizing the current one accordingly
		**/
		if (current->size >= alloc_size + OVERHEAD + FRAGEMENT_THRESHOLD)
		{
			// init new block header
			block_header_t * new = (block_header_t *) ((char *) current + alloc_size);
			new->prev = current;
			new->next = current->next;
			new->size = current->size - alloc_size - OVERHEAD;
			new->is_free = true;

			// now shrink the current one and point next to new
			current->next = new;
			current->size = alloc_size;
		}
	}
	else /* next block does not alread exist */
	{
		// init a new block if we have space
		if (((char *) current) + alloc_size + OVERHEAD <= ((char *) __mem_store_end))
		{
			// init new block header
			current->next = (block_header_t *) ((char *) current + alloc_size);
			current->next->prev = current;
			current->next->next = NULL;
			current->next->size = current->size - alloc_size - OVERHEAD;
			current->next->is_free = true;
		}
		// otherwise mark current as the last one
		else
		{
			current->next = NULL;
		}
	}
	

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
krealloc(void * source, size_t size)
{
	void * region = malloc(size);

	/* 
	 * only copy data from the old section and not of the new lenght
	 * this is for security reasons, so that we dont just read the whole memory to the user
	 * then again this is not a level seperated kernel so lol@me
	**/
	if (region)
	{
		block_header_t * old_region = get_block_header(source);
		memset(region, source, old_region->size);
	}

	kfree(source);
	return region;
}


void
kfree(void * buffer)
{
	block_header_t *header;

	if (!buffer)
		return;

	header = get_block_header(buffer);
	if (!header)
		return;

	// now splice this seciton out
	if (header->next)
		if (header->next->is_free)
			splice_blocks(header, header->next);

	if (header->prev)
		if (header->prev->is_free)
			splice_blocks(header->prev, header);

	header->is_free = true;
}


static block_header_t *
get_block_header(void * buffer)
{
	block_header_t *header = __store_global_head;
	while (header != NULL)
	{
		if ((((block_header_t *) buffer) - 1) == buffer)
			return header;

		header = header->next;
	}
	return NULL;
}


static block_header_t *
splice_blocks(block_header_t * h1, block_header_t * h2)
{
	// change size
	h1->size = h1->size + h2->size + OVERHEAD;

	// splice pointers
	h1->next = h2->next;
	h2->next->prev = h1;
}