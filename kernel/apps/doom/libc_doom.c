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
} MEM_FILE;

FILE *fopen(const char *filename, const char *mode) {
    uint32_t i;
    (void)mode;
    
    if (!filename) return 0;
    
    for (i = 0; i < 10; i++) { // Use static 10 or boot_info.mod_count logic if robust
        // But referencing boot_info.modules safely
        if (i >= boot_info.mod_count) break;
        
        // C89: declarations at top
        boot_module_t *mod = &boot_info.modules[i];
        
        // Exact match on filename
        // mod->string might be "doom1.wad"
        if (mod->string && strcmp((char*)mod->string, filename) == 0) { 
             MEM_FILE *f = (MEM_FILE*)memory_alloc(sizeof(MEM_FILE));
             if (!f) return 0;
             f->data = (uint8_t*)mod->mod_start;
             f->size = mod->mod_end - mod->mod_start;
             f->pos = 0;
             return (FILE*)f;
        }
    }
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

// Basic sprintf implementation
int sprintf(char *str, const char *format, ...) {
    va_list args;
    va_start(args, format);
    char *p = str;
    const char *f = format;
    
    while (*f) {
        if (*f == '%') {
            f++;
            if (*f == 'd') {
                int i = va_arg(args, int);
                if (i < 0) { *p++ = '-'; i = -i; }
                
                char buf[16];
                int j = 0;
                if (i == 0) buf[j++] = '0';
                while (i > 0) {
                    buf[j++] = (i % 10) + '0';
                    i /= 10;
                }
                while (j > 0) {
                    *p++ = buf[--j];
                }
            } else if (*f == 's') {
                char *s = va_arg(args, char*);
                while (*s) *p++ = *s++;
            } else {
                *p++ = *f;
            }
        } else {
            *p++ = *f;
        }
        f++;
    }
    *p = 0;
    va_end(args);
    return (p - str);
}

// Stubs

int rename(const char *old, const char *new) { (void)old; (void)new; return -1; }
int remove(const char *pathname) { (void)pathname; return -1; }
// Old implementations removed
int sscanf(const char *str, const char *format, ...) { (void)str; (void)format; return 0; }
int open(const char *pathname, int flags) { (void)pathname; (void)flags; return -1; }
int close(int fd) { (void)fd; return 0; }
long lseek(int fd, long offset, int whence) { (void)fd; (void)offset; (void)whence; return 0; }
int read(int fd, void *buf, size_t count) { (void)fd; (void)buf; (void)count; return 0; }
int write(int fd, const void *buf, size_t count) { (void)fd; (void)buf; return count; }
int mkdir(const char *pathname, int mode) { (void)pathname; (void)mode; return -1; }
int access(const char *pathname, int mode) { (void)pathname; (void)mode; return -1; } /* Just fail access check */

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
int fstat(int fd, struct stat *buf) { (void)fd; (void)buf; return -1; } // Restored

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

