#ifndef SYSCALL_H
#define SYSCALL_H

// Wrapper functions defined in syscall.asm
extern int syscall_0(int num);
extern int syscall_1(int num, int arg1);
extern int syscall_2(int num, int arg1, int arg2);
extern int syscall_3(int num, int arg1, int arg2, int arg3);
extern int syscall_4(int num, int arg1, int arg2, int arg3, int arg4);
extern int syscall_5(int num, int arg1, int arg2, int arg3, int arg4, int arg5);

// Syscall Numbers
#define SYS_EXIT      1
#define SYS_FORK      2
#define SYS_READ      3
#define SYS_WRITE     4
#define SYS_OPEN      5
#define SYS_CLOSE     6
#define SYS_WAITPID   7
#define SYS_EXECVE    11
#define SYS_MITHL_GUI_CREATE 100
#define SYS_MITHL_GUI_BUTTON 101
#define SYS_MITHL_LOG        102
#define SYS_GET_EVENT        103
#define SYS_DRAW_RECT        104
#define SYS_DRAW_TEXT        105
#define SYS_PROCESS_LIST     106
#define SYS_GET_CMDLINE      107
#define SYS_MKDIR            108
#define SYS_DRAW_IMAGE       109

// Helpers
static inline void sys_exit(int code) {
    syscall_1(SYS_EXIT, code);
}

static inline int sys_write(int fd, const char *buf, int count) {
    return syscall_3(SYS_WRITE, fd, (int)buf, count);
}

static inline int sys_fork(void) {
    return syscall_0(SYS_FORK);
}

static inline int sys_execve(const char *path, char *const argv[], char *const envp[]) {
    return syscall_3(SYS_EXECVE, (int)path, (int)argv, (int)envp);
}

static inline int sys_waitpid(int pid, int *status, int options) {
    return syscall_3(SYS_WAITPID, pid, (int)status, options);
}

#endif
