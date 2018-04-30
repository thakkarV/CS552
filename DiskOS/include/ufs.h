#ifndef UFS_1_0
#define UFS_1_0

#include <types.h>

// block size of 256 bytes
#define UFS_BLOCK_SIZE 0x100

// disk size of 2 MB
#define UFS_DISK_SIZE 0x1FFFFE

// superblock maging number if and when serialized to disk
#define UFS_HEADER_MAGIC 0x1337D00D

// length of inode arrya in number of blocks
#define UFS_SIZEOF_INODE_ARRAY 0x100

// (UFS_SIZEOF_INODE_ARRAY * UFS_BLOCK_SIZE) / sizeof(inode_t)
#define UFS_NUM_MAX_INODES 0x800

// number of blocks for the block bitmap
#define UFS_NUM_BITMAP_BLOCKS 4

// leftover lenght / UFS_BLOCK_SIZE
#define UFS_NUM_MAX_BLOCKS 0xB57

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

typedef enum ufs_blocktype
{
	DIR = 0,
	REG = 1
} ufs_blocktype_t;

typedef enum inode_status
{
    OCCUPIED = 0,
    FREE = 1
} inode_status_t;

typedef struct superblock
{
	uint32_t magic;
	uint32_t num_blocks;
	size_t   block_size;
	uint32_t num_inodes;
	uint32_t num_free_blocks;
	uint32_t num_free_inodes;
	uint32_t num_dirs;
	uint32_t num_files;

	struct inode         *inode_array;
	uint8_t              *blk_bitmap;
	struct ufs_datablock *root_blk;

	// padding at the end to ensure size of 64 bytes
	uint32_t padding[53];
} __attribute__((packed)) ufs_superblock_t;

// SIZEOF(inode) = 64 BYTES
typedef struct inode
{
	/* inode_t layout
	 *	+------+------+-+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+--+
	 *	| Type | Size | 8 Direct BlkPtr | 1 Indirect BlkPtr | 1 Double Indirect BlkPtr |
	 *	+------+------+-+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+--+
	**/
	ufs_blocktype_t type;
	size_t size;
	uint32_t attrib;
	struct ufs_datablock *direct_block_ptrs[8];
	struct ufs_datablock **indirect_block_ptr;
	struct ufs_datablock ***double_indirect_block_ptr;

	// padding at the end to ensure size of 64 bytes
	uint32_t padding[3];
} __attribute__((packed)) inode_t;


typedef struct ufs_dirent
{
	char filename[14];
	uint16_t inode_num;
} __attribute__((packed)) ufs_dirent_t;

typedef struct ufs_dirblock
{
	// number of dir entries is equal to = FS_BLOCK_SIZE / sizeof(dirent)
	struct ufs_dirent entries[16];
} __attribute__((packed)) ufs_dirblock_t;


typedef struct ufs_datablock
{
	char data[UFS_BLOCK_SIZE];
} __attribute__((packed)) ufs_datablock_t;

#endif // UFS_1_0
