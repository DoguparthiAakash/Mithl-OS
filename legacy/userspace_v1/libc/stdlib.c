#include "stdlib.h"

// Defined in syscall.asm
extern int syscall_0(int num);
extern int syscall_1(int num, int arg1);
extern int syscall_3(int num, int arg1, int arg2, int arg3);

#define SYS_EXIT      1
#define SYS_WRITE     4
#define SYS_MITHL_LOG 102
#define SYS_MITHL_GUI_CREATE 100

void exit(int status) {
    syscall_1(SYS_EXIT, status);
    while(1);
}

void print(const char *msg) {
    syscall_1(SYS_MITHL_LOG, (int)msg);
}

int create_window(const char *title, int x, int y, int w, int h) {
    // We need 5 arguments.
    // Our syscall.asm only has syscall_3.
    // Let's rely on registers being preserved or add syscall_5.
    // Actually, let's implement syscall_5 quickly in asm.
    extern int syscall_5(int num, int arg1, int arg2, int arg3, int arg4, int arg5);
    return syscall_5(SYS_MITHL_GUI_CREATE, (int)title, x, y, w, h);
}

#define SYS_EXECVE 11
int execve(const char *filename, char *const argv[], char *const envp[]) {
    return syscall_3(SYS_EXECVE, (int)filename, (int)argv, (int)envp);
}
