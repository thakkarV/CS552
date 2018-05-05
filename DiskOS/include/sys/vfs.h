#ifndef SYS_VFS
#define SYS_VFS
#include <sys/ufs.h>

#define NUM_MAX_FD 0x10

int get_avail_fd(void);

typedef struct FILE
{
    int fd;
    size_t seek_head;
    char * path;
    inode_t * inode_ptr;
} __attribute__((packed)) FILE;

#endif // SYS_VFS