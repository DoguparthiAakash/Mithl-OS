#ifndef _FCNTL_H
#define _FCNTL_H

#define O_RDONLY 0
#define O_WRONLY 1
#define O_RDWR   2
#define O_CREAT  0x0200
#define O_TRUNC  0x0400
#define O_APPEND 0x0008
#define O_BINARY 0x8000 // For Doom

int open(const char *pathname, int flags, ...);

#endif
