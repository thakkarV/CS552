#ifndef SYS_KMALLOC
#define SYS_KMALLOC

#include <sys/multiboot.h>
#include <sys/types.h>

/* NOTE : BLOCK SIZE DOES NOT INCLUDE BLOCK HEADER METADATA VOERHEAD */
struct block_header {
	struct block_header *prev;
	struct block_header *next;
	size_t size;
	int is_free;
} __attribute__((packed));
typedef struct block_header block_header_t;

/* OS initialization routine for the dynamic allocator
 * two inputs are the base addresses for the mem regions
 * first is the base addr for the book-keeping linked list
 * second is the free store base addr
 **/
// void init_kmalloc(void * list_addr, size_t list_max_size);
void init_kmalloc(multiboot_info_t *);

/* malloc interface */
void *kmalloc(size_t);
void *kcalloc(size_t);
void *krealloc(void *, size_t);
void kfree(void *);

#endif // KMALLOC
