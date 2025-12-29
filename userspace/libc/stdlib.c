#include "stdlib.h"

#include "stdlib.h"
#include "syscall.h" // Include syscall numbers
#include "string.h"

// Defined in syscall.asm
extern int syscall_0(int num);
extern int syscall_1(int num, int arg1);
extern int syscall_2(int num, int arg1, int arg2);
extern int syscall_3(int num, int arg1, int arg2, int arg3);

// Removed manual SYS defines (using syscall.h)

// Errno
int errno = 0;

// Setjmp Stubs
#include "setjmp.h" // Assuming user added -I./libc
int setjmp(jmp_buf env) { return 0; }
void longjmp(jmp_buf env, int val) {
    print("FATAL: longjmp called (unsupported). Exiting.\n");
    exit(1);
}

// Math Stubs
#include "math.h"
double floor(double x) { return (long)x; }
double ceil(double x) { return (long)x + 1; }
double pow(double x, double y) { return 1.0; } // Mock
double strtod(const char *nptr, char **endptr) {
    // Minimal parser or 0
    if(endptr) *endptr = (char*)nptr;
    return 0.0;
}
long double strtold(const char *nptr, char **endptr) {
    if(endptr) *endptr = (char*)nptr;
    return 0.0;
}

// Time Stubs
#include "time.h"
#include "sys/time.h"
time_t time(time_t *tLoc) { return 0; }
int gettimeofday(struct timeval *tv, void *tz) {
    if(tv) { tv->tv_sec = 0; tv->tv_usec = 0; }
    return 0;
}

// Also need dlopen/dlsym if included... but TCC_STATIC should hide it.

void exit(int status) {
    syscall_1(SYS_EXIT, status);
    while(1);
}

// Mmap Args
struct mmap_args {
    uint32_t addr;
    uint32_t len;
    uint32_t prot;
    uint32_t flags;
    uint32_t fd;
    uint32_t offset;
};

// Mmap
void *mmap(void *addr, size_t length, int prot, int flags, int fd, int offset) {
    struct mmap_args args;
    args.addr = (uint32_t)addr;
    args.len = (uint32_t)length;
    args.prot = (uint32_t)prot;
    args.flags = (uint32_t)flags;
    args.fd = (uint32_t)fd;
    args.offset = (uint32_t)offset;
    
    // Sycall 90 (SYS_MMAP) takes pointer to args in EBX
    return (void*)syscall_1(90, (int)&args);
}

int munmap(void *addr, size_t length) {
    return syscall_2(91, (int)addr, length);
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

#define SYS_DRAW_IMAGE 109
void draw_image(const uint32_t *data, int x, int y, int w, int h) {
    extern int syscall_5(int num, int arg1, int arg2, int arg3, int arg4, int arg5);
    syscall_5(SYS_DRAW_IMAGE, (int)data, x, y, w, h);
}

// File System
#define SYS_READ      3
#define SYS_OPEN      5
#define SYS_CHDIR 14
#define SYS_CLOSE     6
#define SYS_READDIR   12

int open(const char *path, int flags, ...) {
    // We ignore mode for now or pass 0
    return syscall_3(SYS_OPEN, (int)path, flags, 0);
}

int close(int fd) {
    return syscall_1(SYS_CLOSE, fd);
}

int write(int fd, const void *buf, uint32_t count) {
    return syscall_3(SYS_WRITE, fd, (int)buf, count);
}

// Stdin Support
int lseek(int fd, int offset, int whence) {
    return syscall_3(SYS_LSEEK, fd, offset, whence);
}

int read(int fd, void *buf, uint32_t count) {
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
    return syscall_2(38, (uint32_t)oldpath, (uint32_t)newpath);
}


int chdir(const char *path) {
    return syscall_1(SYS_CHDIR, (uint32_t)path);
}

#define SYS_CREAT 8
int creat(const char *pathname, uint32_t mode) {
    return syscall_3(SYS_CREAT, (int)pathname, mode, 0);
}

#define SYS_FORK 2
int fork() {
    return syscall_0(SYS_FORK);
}

int waitpid(int pid, int *status, int options) {
    return syscall_3(SYS_WAITPID, pid, (int)status, options);
}

int execve(const char *filename, char *const argv[], char *const envp[]) {
    return syscall_3(SYS_EXECVE, (uint32_t)filename, (uint32_t)argv, (uint32_t)envp);
}

int pipe(int pipefd[2]) {
    return syscall_1(SYS_PIPE, (uint32_t)pipefd);
}

int dup2(int oldfd, int newfd) {
    return syscall_2(SYS_DUP2, (uint32_t)oldfd, (uint32_t)newfd);
}

int agent_op(int op, void *arg1, void *arg2) {
    return syscall_3(SYS_AGENT_OP, op, (uint32_t)arg1, (uint32_t)arg2);
}

// TCC Requirements
void qsort(void *base, size_t nmemb, size_t size, int (*compar)(const void *, const void *)) {
    // Bubble sort for simplicity (slow but small)
    if (nmemb < 2 || size == 0) return;
    char *b = (char*)base;
    void *tmp = malloc(size);
    for (size_t i = 0; i < nmemb - 1; i++) {
        for (size_t j = 0; j < nmemb - i - 1; j++) {
            if (compar(b + j * size, b + (j + 1) * size) > 0) {
                memcpy(tmp, b + j * size, size);
                memcpy(b + j * size, b + (j + 1) * size, size);
                memcpy(b + (j + 1) * size, tmp, size);
            }
        }
    }
    free(tmp);
}

unsigned long strtoul(const char *nptr, char **endptr, int base) {
    // Minimal decimal/hex parser
    unsigned long val = 0;
    while(*nptr == ' ') nptr++;
    if (*nptr == '0' && (nptr[1] == 'x' || nptr[1] == 'X')) {
        nptr += 2; base = 16;
    }
    if (base == 0) base = 10;
    
    while(*nptr) {
        int d = -1;
        if (*nptr >= '0' && *nptr <= '9') d = *nptr - '0';
        else if (*nptr >= 'a' && *nptr <= 'f') d = *nptr - 'a' + 10;
        else if (*nptr >= 'A' && *nptr <= 'F') d = *nptr - 'A' + 10;
        
        if (d >= 0 && d < base) {
            val = val * base + d;
            nptr++;
        } else break;
    }
    if (endptr) *endptr = (char*)nptr;
    return val;
}

long strtol(const char *nptr, char **endptr, int base) {
    return (long)strtoul(nptr, endptr, base);
}

long long strtoll(const char *nptr, char **endptr, int base) {
    return (long long)strtoul(nptr, endptr, base);
}

unsigned long long strtoull(const char *nptr, char **endptr, int base) {
    return (unsigned long long)strtoul(nptr, endptr, base);
}

char *getenv(const char *name) {
    return 0; // NULL
}

double ldexpl(double x, int exp) {
    // x * 2^exp
    // Minimal stub
     if (exp == 0) return x;
     return x; 
}

// Time
struct tm *localtime(const time_t *timep) {
    static struct tm t;
    // Always return zero time
    memset(&t, 0, sizeof(t));
    return &t;
}

// mprotect
int mprotect(void *addr, size_t len, int prot) {
    return 0;
}

char **environ = NULL;

int atoi(const char *nptr) {
    return (int)strtol(nptr, NULL, 10);
}

char *realpath(const char *path, char *resolved_path) {
    // Stub: just copy path
    if (!resolved_path) resolved_path = malloc(256); // unsafe size
    strcpy(resolved_path, path);
    return resolved_path;
}

#include "string.h"

int execvp(const char *file, char *const argv[]) {
    // Stub: no PATH search
    return -1;
}

// Stubs for TCC
char *getcwd(char *buf, size_t size) {
    if (buf) strcpy(buf, "/");
    return buf;
}

float strtof(const char *nptr, char **endptr) {
    return (float)strtod(nptr, endptr);
}

// 64-bit Division Stubs (Temporary)
unsigned long long __udivdi3(unsigned long long n, unsigned long long d) {
    // print("Warning: __udivdi3 called\n");
    if (d == 0) return 0;
    // Hack: cast to 32 bit if it fits
    if (n < 0xFFFFFFFF && d < 0xFFFFFFFF) return (unsigned int)n / (unsigned int)d;
    return 0; 
}

unsigned long long __umoddi3(unsigned long long n, unsigned long long d) {
    if (d == 0) return 0;
    if (n < 0xFFFFFFFF && d < 0xFFFFFFFF) return (unsigned int)n % (unsigned int)d;
    return 0;
}
