#include "libc_shim/stdio.h"
#include "libc_shim/stdlib.h"
#include "libc_shim/ctype.h"
#include "string.h"
#include "boot_info.h"
#include "console.h"
#include "memory.h"
#include "sys/stat.h"

// Defined in kernel.c
extern boot_info_t boot_info;

typedef struct {
    uint8_t *data;
    uint32_t size;
    uint32_t pos;
    int used; // For FD table
} MEM_FILE;

static MEM_FILE fd_table[32]; // Simple FD table
static int fd_table_init = 0;

void init_fd_table() {
    if (!fd_table_init) {
        memset(fd_table, 0, sizeof(fd_table));
        fd_table_init = 1;
    }
}


// Prototypes
int strncasecmp(const char *s1, const char *s2, size_t n);
size_t strlen(const char *s);

// Helper: Check if module string matches requested filename (basename) case-insensitive
static int module_matches_filename(const char *mod_str, const char *filename) {
    if (!mod_str || !filename) return 0;
    
    // Extract basename
    const char *base = filename;
    const char *p = filename;
    while (*p) {
        if (*p == '/' || *p == '\\') base = p + 1;
        p++;
    }
    
    size_t base_len = strlen(base);
    if (base_len == 0) return 0;

    // Search for basename in mod_str (case insensitive)
    const char *curr = mod_str;
    while (*curr) {
        if (strncasecmp(curr, base, base_len) == 0) {
            return 1;
        }
        curr++;
    }
    return 0;
}

FILE *fopen(const char *filename, const char *mode) {
    uint32_t i;
    (void)mode;
    
    if (!filename) return 0;
    
    serial_write("[DOOM] fopen: ");
    serial_write(filename);
    
    // Search modules
    for (i = 0; i < 10; i++) { 
        if (i >= boot_info.mod_count) break;
        
        boot_module_t *mod = &boot_info.modules[i];
        
        if (mod->string && module_matches_filename((char*)mod->string, filename)) { 
             serial_write(" -> Found in module: ");
             serial_write((char*)mod->string);
             serial_write("\n");
             
             MEM_FILE *f = (MEM_FILE*)memory_alloc(sizeof(MEM_FILE));
             if (!f) return 0;
             f->data = (uint8_t*)mod->mod_start;
             f->size = mod->mod_end - mod->mod_start;
             f->pos = 0;
             return (FILE*)f;
        }
    }
    
    serial_write(" -> Not found\n");
    return 0;
}

int fclose(FILE *stream) {
    if (stream) {
        memory_free(stream);
    }
    return 0;
}

size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    MEM_FILE *f = (MEM_FILE*)stream;
    long bytes = size * nmemb;
    if (!f) return 0;
    
    if (f->pos + bytes > f->size) {
        bytes = f->size - f->pos;
    }
    if (bytes <= 0) return 0;
    
    memcpy(ptr, f->data + f->pos, bytes);
    f->pos += bytes;
    
    return bytes / size;
}

int fseek(FILE *stream, long offset, int whence) {
    MEM_FILE *f = (MEM_FILE*)stream;
    if (!f) return -1;
    
    if (whence == SEEK_SET) {
        f->pos = offset;
    } else if (whence == SEEK_CUR) {
        f->pos += offset;
    } else if (whence == SEEK_END) {
        f->pos = f->size + offset;
    }
    
    if (f->pos > f->size) f->pos = f->size;
    // f->pos is unsigned, so < 0 check is tricky if types mismatch. 
    // But MEM_FILE.pos is uint32_t. offset is long.
    // Ideally clamp properly.
    
    return 0;
}

long ftell(FILE *stream) {
    MEM_FILE *f = (MEM_FILE*)stream;
    if (!f) return -1;
    return f->pos;
}

// Core vsprintf implementation
int vsprintf(char *str, const char *format, va_list args) {
    char *p = str;
    const char *f = format;
    
    while (*f) {
        if (*f == '%') {
            f++;
            if (*f == 's') {
                char *s = va_arg(args, char*);
                if (!s) s = "(null)";
                while (*s) *p++ = *s++;
            } else if (*f == 'd' || *f == 'i') {
                int val = va_arg(args, int);
                char buf[32];
                int i = 0;
                if (val == 0) buf[i++] = '0';
                else {
                    int sign = 0;
                    if (val < 0) { sign = 1; val = -val; }
                    while (val > 0) {
                        buf[i++] = '0' + (val % 10);
                        val /= 10;
                    }
                    if (sign) buf[i++] = '-';
                }
                while (i > 0) *p++ = buf[--i];
            } else if (*f == 'x' || *f == 'X') {
                uint32_t val = va_arg(args, uint32_t);
                char hexbuf[16];
                int j = 0;
                if (val == 0) hexbuf[j++] = '0';
                while (val > 0) {
                    int digit = val % 16;
                    if (digit < 10) hexbuf[j++] = '0' + digit;
                    else hexbuf[j++] = 'A' + (digit - 10);
                    val /= 16;
                }
                while (j > 0) *p++ = hexbuf[--j];
            } else if (*f == 'u') { 
                 uint32_t val = va_arg(args, uint32_t);
                 char buf[32];
                 int i = 0;
                 if (val == 0) buf[i++] = '0';
                 else {
                     while (val > 0) {
                         buf[i++] = '0' + (val % 10);
                         val /= 10;
                     }
                 }
                 while (i > 0) *p++ = buf[--i];
            } else {
                *p++ = *f;
            }
        } else {
            *p++ = *f;
        }
        f++;
    }
    *p = 0;
    return (p - str);
}

int sprintf(char *str, const char *format, ...) {
    va_list args;
    va_start(args, format);
    int ret = vsprintf(str, format, args);
    va_end(args);
    return ret;
}

// External hooks
extern void serial_write(const char *s);
extern void console_write(const char *s);

int printf(const char *format, ...) {
    char buf[1024];
    va_list args;
    va_start(args, format);
    int ret = vsprintf(buf, format, args);
    va_end(args);
    
    serial_write(buf); 
    // console_write(buf); // Optional, might clutter screen logic
    return ret;
}

int fprintf(FILE *stream, const char *format, ...) {
    (void)stream; // Ignore stream, print to debug
    char buf[1024];
    va_list args;
    va_start(args, format);
    int ret = vsprintf(buf, format, args);
    va_end(args);
    
    serial_write(buf); 
    return ret;
}

int vfprintf(FILE *stream, const char *format, va_list ap) {
    (void)stream;
    char buf[1024];
    int ret = vsprintf(buf, format, ap);
    serial_write(buf);
    return ret;
}

int fflush(FILE *stream) {
    (void)stream;
    return 0;
}

// Stubs

int rename(const char *old, const char *new) { (void)old; (void)new; return -1; }
int remove(const char *pathname) { (void)pathname; return -1; }
// Old implementations removed
int sscanf(const char *str, const char *format, ...) { (void)str; (void)format; return 0; }
// File Descriptor Implementation for w_wad.c (which uses open/read/close)
int open(const char *pathname, int flags) {
    int i;
    int fd;
    (void)flags;
    init_fd_table();
    
    if (!pathname) return -1;
    
    serial_write("[DOOM] open: ");
    serial_write(pathname);

    // Search modules
    for (i = 0; i < 10; i++) { 
        if ((uint32_t)i >= boot_info.mod_count) break;
        boot_module_t *mod = &boot_info.modules[i];
        
        if (mod->string && module_matches_filename((char*)mod->string, pathname)) {
             serial_write(" -> Success (fd ");

             // Find free FD
             for (fd = 3; fd < 32; fd++) {
                 if (!fd_table[fd].used) {
                     fd_table[fd].used = 1;
                     fd_table[fd].data = (uint8_t*)mod->mod_start;
                     fd_table[fd].size = mod->mod_end - mod->mod_start;
                     fd_table[fd].pos = 0;
                     
                     // Debug Address
                     char pbuf[32];
                     sprintf(pbuf, " Addr:%x Sz:%d", (uint32_t)mod->mod_start, fd_table[fd].size);
                     serial_write(pbuf);

                     // Sanity Check Header
                     
                     // Sanity Check Header
                     if (fd_table[fd].size > 4) {
                         uint32_t *hdr_ptr = (uint32_t*)fd_table[fd].data;
                         char hbuf[32];
                         sprintf(hbuf, " [HDR-HEX: %x] ", *hdr_ptr);
                         serial_write(hbuf);
                     }
                     
                     // Quick dumb int to string
                     char buf[4];
                     buf[0] = '0' + (fd / 10); 
                     buf[1] = '0' + (fd % 10);
                     buf[2] = 0;
                     serial_write(" fd:");
                     serial_write(buf);
                     serial_write("\n");
                     
                     return fd;
                 }
             }
             serial_write(" -> ERRO: No free FDs\n");
             return -1;
        }
    }
    serial_write(" -> Not found\n");
    return -1;
}

int close(int fd) { 
    if (fd >= 3 && fd < 32) {
        fd_table[fd].used = 0;
        return 0;
    }
    return -1; 
}

long lseek(int fd, long offset, int whence) { 
    if (fd < 3 || fd >= 32 || !fd_table[fd].used) return -1;
    MEM_FILE *f = &fd_table[fd];
    
    if (whence == SEEK_SET) f->pos = offset;
    else if (whence == SEEK_CUR) f->pos += offset;
    else if (whence == SEEK_END) f->pos = f->size + offset;
    
    if (f->pos > f->size) f->pos = f->size; // Clamp?
    return f->pos;
}

int read(int fd, void *buf, size_t count) { 
    if (fd < 3 || fd >= 32 || !fd_table[fd].used) return -1;
    MEM_FILE *f = &fd_table[fd];
    
    if (f->pos + count > f->size) count = f->size - f->pos;
    if ((int)count <= 0) return 0;
    
    memcpy(buf, f->data + f->pos, count);
    f->pos += count;
    return count; 
}

// Needed by w_wad.c:filelength()
int fstat(int fd, struct stat *buf) {
    if (fd < 3 || fd >= 32 || !fd_table[fd].used) return -1;
    if (buf) {
        buf->st_size = fd_table[fd].size;
        return 0;
    }
    return -1;
}

int write(int fd, const void *buf, size_t count) { 
    // Stdout/Stderr
    if (fd == 1 || fd == 2) {
        size_t i;
        char *str = (char*)buf;
        for (i=0; i<count; i++) console_putc(str[i]);
        return count;
    }
    return -1; // Read only modules
}
int mkdir(const char *pathname, int mode) { (void)pathname; (void)mode; return -1; }
int access(const char *pathname, int mode) { 
    (void)mode;
    
    if (!pathname) return -1;
    
    serial_write("[DOOM] access: ");
    serial_write(pathname);

    // Search modules for file existence
    uint32_t i;
    for (i = 0; i < 10; i++) {
        if (i >= boot_info.mod_count) break;
        boot_module_t *mod = &boot_info.modules[i];
        
        if (mod->string) {
             serial_write(" Checking mod: '");
             serial_write((char*)mod->string);
             serial_write("' ... ");
             
             if (module_matches_filename((char*)mod->string, pathname)) {
                 serial_write("MATCH!\n");
                 return 0;
             }
             serial_write("No match.\n");
        }
    }

    serial_write(" -> Not found\n");
    return -1;
}


#include "libc_shim/stdio.h"
#include "libc_shim/stdlib.h"
#include "libc_shim/ctype.h" // Added
#include "string.h"
#include "boot_info.h"
#include "console.h"
#include "memory.h"
#include "sys/stat.h" // For struct stat

// ...

int stat(const char *path, struct stat *buf) { (void)path; (void)buf; return -1; }
// int stat removed (duplicate)

// fstat moved up


int putchar(int c) { console_putc(c); return c; }
int puts(const char *s) { console_write(s); console_write("\n"); return 1; }

void setbuf(FILE *stream, char *buf) { (void)stream; (void)buf; }
int getchar(void) { return 0; } // Stub
int fscanf(FILE *stream, const char *format, ...) { 
    (void)stream; (void)format; 
    return 0; // EOF or 0
}

int feof(FILE *stream) { 
    (void)stream; 
    return 1; // Always EOF for now
}

int strcasecmp(const char *s1, const char *s2) {
    while (*s1 && *s2) {
        int c1 = tolower(*s1);
        int c2 = tolower(*s2);
        if (c1 != c2) return c1 - c2;
        s1++; s2++;
    }
    return tolower(*s1) - tolower(*s2);
}

int strncasecmp(const char *s1, const char *s2, size_t n) {
    if (n == 0) return 0;
    while (n-- != 0 && *s1 && *s2) {
        int c1 = tolower(*s1);
        int c2 = tolower(*s2);
        if (c1 != c2) return c1 - c2;
        s1++; s2++;
    }
    if (n == (size_t)-1) return 0; // Matched n chars
    // If one ended before n
    return tolower(*s1) - tolower(*s2);
}

// Global variable for doom
char *sndserver_filename = "./sndserver";

// Missing HAL logic
void I_SetMusicVolume(int volume) { (void)volume; }

// Environment variable support (fake for DOOM)
char *getenv(const char *name) {
    if (strcmp(name, "HOME") == 0) {
        return "/home/user";  // Fake home directory
    }
    if (strcmp(name, "USER") == 0) {
        return "user";
    }
    return NULL;  // Not found
}
