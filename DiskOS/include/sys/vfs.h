#ifndef SYS_VFS
#define SYS_VFS

#include <sys/ufs.h>

#define NUM_MAX_FD 0x10

typedef struct FILE
{
    int fd;
    size_t seek_head;
    char path[14];
    inode_t * inode_ptr;
} __attribute__((packed)) FILE;

typedef struct file_descriptor_table
{
    FILE * proc_fds[NUM_MAX_FD];
} __attribute__((packed)) fd_table_t;

#endif // SYS_VFS