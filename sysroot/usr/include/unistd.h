#ifndef UNISTD_H
#define UNISTD_H

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

// Standard File Descriptors
#define STDIN_FILENO  0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

// Prototypes
int open(const char *path, int flags, ...);
int close(int fd);
int read(int fd, void *buf, size_t count);
int write(int fd, const void *buf, size_t count);
int lseek(int fd, int offset, int whence);
int unlink(const char *pathname);
int chdir(const char *path);
char *getcwd(char *buf, size_t size);

int fork(void);
int execve(const char *pathname, char *const argv[], char *const envp[]);
int execvp(const char *file, char *const argv[]);
void _exit(int status);
int getpid(void);

void *sbrk(intptr_t increment);
int brk(void *addr);

unsigned int sleep(unsigned int seconds);

#endif
