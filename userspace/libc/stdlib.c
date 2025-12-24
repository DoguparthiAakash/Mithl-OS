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


int create_window(const char *title, int x, int y, int w, int h) {
    // We need 5 arguments.
    // Our syscall.asm only has syscall_3.
    // Let's rely on registers being preserved or add syscall_5.
    // Actually, let's implement syscall_5 quickly in asm.
    extern int syscall_5(int num, int arg1, int arg2, int arg3, int arg4, int arg5);
    return syscall_5(SYS_MITHL_GUI_CREATE, (int)title, x, y, w, h);
}

// Duplicate definition removed from here

#define SYS_GET_EVENT 103
int get_event(gui_event_t *event) {
    return syscall_1(SYS_GET_EVENT, (int)event);
}

#define SYS_DRAW_RECT 104
void draw_rect(int x, int y, int w, int h, uint32_t color) {
    // syscall_5(num, x, y, w, h, color)
    extern int syscall_5(int num, int arg1, int arg2, int arg3, int arg4, int arg5);
    syscall_5(SYS_DRAW_RECT, x, y, w, h, color);
}

#define SYS_DRAW_TEXT 105
void draw_text(const char *msg, int x, int y, uint32_t color) {
    // syscall_5 for convenience (unused args 5) or syscall_3?
    // kernel expects: ebx=msg, ecx=x, edx=y, esi=color.
    // syscall_5: num, arg1(ebx), arg2(ecx), arg3(edx), arg4(esi), arg5(edi)
    // kernel handler: msg=ebx, x=ecx, y=edx, color=esi.
    
    // So arguments match standard.
    // BUT! syscall_3 only passes ebx, ecx, edx.
    // We need 4 args (msg, x, y, color).
    // syscall_5 works, but we need 4 args.
    // Let's use syscall_5 and pass 0 for arg5.
    extern int syscall_5(int num, int arg1, int arg2, int arg3, int arg4, int arg5);
    syscall_5(SYS_DRAW_TEXT, (int)msg, x, y, color, 0);
}

// File System
#define SYS_READ      3
#define SYS_OPEN      5
#define SYS_CLOSE     6
#define SYS_READDIR   12

int open(const char *path, int flags) {
    return syscall_3(SYS_OPEN, (int)path, flags, 0);
}

int close(int fd) {
    return syscall_1(SYS_CLOSE, fd);
}

int write(int fd, const void *buf, int count) {
    return syscall_3(SYS_WRITE, fd, (int)buf, count);
}

// Stdin Support
int read(int fd, void *buf, int count) {
    return syscall_3(SYS_READ, fd, (int)buf, count);
}

char getchar() {
    char c = 0;
    read(0, &c, 1);
    return c;
}

void gets(char *buf) {
    int i = 0;
    while(1) {
        char c = getchar();
        if (c == '\n') {
            buf[i] = 0;
            // Echo newline?
            // console_write("\n"); // syscall-based print actually
            print("\n");
            return;
        }
        if (c == '\b') {
            if (i > 0) {
                i--;
                // print space backspace hack?
                print("\b \b"); // Erase char on console
            }
        } else if (c) {
            buf[i++] = c;
            // Echo char
            char str[2] = {c, 0};
            print(str);
        }
    }
}

void print(const char *s) {
    int len = 0; while(s[len]) len++;
    write(1, s, len); // Use write to stdout
}

static dirent_t readdir_buf;
dirent_t *readdir(int fd) {
    int res = syscall_3(SYS_READDIR, fd, (int)&readdir_buf, 0);
    if (res == 1) {
        return &readdir_buf;
    }
    return 0; // NULL on EOF or Error
}

#define SYS_PROCESS_LIST 106
int get_process_list(process_info_t *buf, int max_count) {
    return syscall_3(SYS_PROCESS_LIST, (int)buf, max_count, 0);
}

#define SYS_GET_CMDLINE 107
int get_cmdline(char *buf, int max_len) {
    return syscall_3(SYS_GET_CMDLINE, (int)buf, max_len, 0);
}

#define SYS_MKDIR 108
int mkdir(const char *path, uint32_t mode) {
    return syscall_3(SYS_MKDIR, (int)path, mode, 0);
}

#define SYS_UNLINK 10
int unlink(const char *pathname) {
    return syscall_3(SYS_UNLINK, (int)pathname, 0, 0);
}

#define SYS_RENAME 38
int rename(const char *oldpath, const char *newpath) {
    return syscall_3(SYS_RENAME, (int)oldpath, (int)newpath, 0);
}

#define SYS_CREAT 8
int creat(const char *pathname, uint32_t mode) {
    return syscall_3(SYS_CREAT, (int)pathname, mode, 0);
}

#define SYS_FORK 2
int fork() {
    return syscall_0(SYS_FORK);
}

#define SYS_WAITPID 7
int waitpid(int pid, int *status, int options) {
    return syscall_3(SYS_WAITPID, pid, (int)status, options);
}

#define SYS_EXECVE 11
int execve(const char *filename, char *const argv[], char *const envp[]) {
    return syscall_3(SYS_EXECVE, (int)filename, (int)argv, (int)envp);
}
