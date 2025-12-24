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
