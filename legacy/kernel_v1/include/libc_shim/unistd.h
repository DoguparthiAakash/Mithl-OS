#ifndef _UNISTD_H
#define _UNISTD_H

#include <sys/types.h>
#include <stdio.h> // for SEEK_SET if needed

int close(int fd);
long lseek(int fd, long offset, int whence);
int read(int fd, void *buf, size_t count);
int write(int fd, const void *buf, size_t count);
int access(const char *pathname, int mode);

#define R_OK 4
#define W_OK 2
#define X_OK 1
#define F_OK 0

#endif
