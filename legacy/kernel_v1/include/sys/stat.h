#ifndef _SYS_STAT_H
#define _SYS_STAT_H

#include <sys/types.h>

struct stat {
    int st_dev;
    int st_ino;
    int st_mode;
    int st_nlink;
    int st_uid;
    int st_gid;
    int st_rdev;
    long st_size;
    long st_atime;
    long st_mtime;
    long st_ctime;
};

#define S_IFMT  0170000
#define S_IFDIR 0040000
#define S_IFCHR 0020000
#define S_IFBLK 0060000
#define S_IFREG 0100000
#define S_IFIFO 0010000
#define S_IFLNK 0120000
#define S_IFSOCK 0140000

#define S_ISDIR(m)  (((m) & S_IFMT) == S_IFDIR)

int stat(const char *path, struct stat *buf);
int fstat(int fd, struct stat *buf);
int mkdir(const char *pathname, mode_t mode);

#endif
