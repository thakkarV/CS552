#ifndef UFS_RAMDISK
#define UFS_RAMDISK

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
