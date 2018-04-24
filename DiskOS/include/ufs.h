#ifndef UFS_1_0
#define UFS_1_0

#include <types.h>

// block size of 256 bytes
#define FS_BLOCK_SIZE 256

// disk size of 2 MB
#define FS_DISK_SIZE 0xFFFFF

/* LOGICAL LAYOUT OF UNIX FILESYSTEM - 256 byte block size
I) Superblock
	blk number 0

II) Index Nodes Array
	256 Blks if index nodes (inode_t) structs containing file attributes
	Each inode is 64 bytes in size. layout of the node is described in the struct code

III) Block Bitmap
	After index blocks are 4 blocks of block bitmaps used to track allocated and unallocated blocks in the fs

IV) Data
	Rest of the 2 MB holds the files stores in the file system.
	Files could be either direcotry entries or regular files.
	-> Regualr files hold arbitraty data
	-> dirents hold a touple of (filename, inode number) pair
		--> filename is a null padded 14 byte string
		--> inode number is a 16 bit int
*/

typedef enum fs_blocktype
{
	DIR = 0,
	REG = 1
} fs_blocktype_t;


typedef struct superblock
{
	uint32_t magic;
	uint32_t num_blocks;
	size_t   block_size;
	uint32_t num__inodes;
	uint32_t num_free_blocks;
	uint32_t num_free_inodes;

} __attribute__((packed)) superblock_t;


typedef struct inode
{
	/* inode_t layout
	 *	+------+------+-+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+--+
	 *	| Type | Size | 8 Direct BlkPtr | 1 Indirect BlkPtr | 1 Double Indirect BlkPtr |
	 *	+------+------+-+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+--+
	**/
	fs_blocktype_t type;
	size_t size;
	struct fs_datablock_t * direct_block_ptrs[8];
	struct fs_datablock_t ** indirect_block_ptr;
	struct fs_datablock_t *** double_indirect_block_ptr;
} __attribute__((packed)) inode_t;


typedef struct fs_dirent
{
	char filename[14];
	uint16_t inode_num;
} __attribute__((packed)) fs_dirent_t;

typedef struct fs_dirblock
{
	// number of dir entries is equal to = FS_BLOCK_SIZE / sizeof(dirent)
	struct fs_dirent entries[16];
} __attribute__((packed)) fs_dirblock_t;


typedef struct fs_datablock
{
	char data[FS_BLOCK_SIZE];
} __attribute__((packed)) fs_datablock_t;

#endif // UFS_1_0
