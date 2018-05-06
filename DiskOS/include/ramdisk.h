#ifndef UFS_RAMDISK
#define UFS_RAMDISK

#define EBADF     -1 // bad fd
#define EFAULT    -2 // buf outside process address space
#define EBADBUF   -3 // bad buffer address
#define EBOUNDS   -4 // trying to read or beyond file end/max size
#define EBADPATH  -5 // bad input path
#define EMAXF     -6 // out of allocatable file discriptors

void init_rdisk(void *);

// FILE OPERATIONS
int rd_create(char *);
int rd_mkdir(char *);
int rd_open(char *);
int rd_close(int fd);
int rd_read(int, char *, int);
int rd_write(int, char *, int);
int rd_lseek(int, int);
int rd_unlink(char *);
int rd_readdir(int, char *);

#endif // UFS_RAMDISK
