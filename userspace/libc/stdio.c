#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include <stdarg.h>

// Syscall wrappers from stdlib.h/syscall.h
extern int open(const char *path, int flags);
extern int close(int fd);
extern int read(int fd, void *buf, uint32_t count);
extern int write(int fd, const void *buf, uint32_t count);
extern int lseek(int fd, int offset, int whence); // Missing in stdlib.h, need to check syscalls

// Internal FILE objects for standard streams
static FILE _stdin = {0, 0, 0};
static FILE _stdout = {1, 0, 0};
static FILE _stderr = {2, 0, 0};

FILE *stdin = &_stdin;
FILE *stdout = &_stdout;
FILE *stderr = &_stderr;

FILE *fopen(const char *pathname, const char *mode) {
    // Flags mapping (simplified)
    // "r" -> O_RDONLY
    // "w" -> O_WRONLY | O_CREAT | O_TRUNC
    // 0=RDONLY, 1=WRONLY, 2=RDWR
    // Userspace open syscall: open(path, flags)
    // We assume O_RDONLY=0 for now.
    
    int flags = 0;
    if (strchr(mode, 'w')) flags = 1; // write
    
    int fd = open(pathname, flags);
    if (fd < 0) return NULL;
    
    FILE *f = (FILE*)malloc(sizeof(FILE));
    f->fd = fd;
    f->error = 0;
    f->eof = 0;
    return f;
}

int fclose(FILE *stream) {
    if (!stream) return -1;
    int res = close(stream->fd);
    if (stream != stdin && stream != stdout && stream != stderr) {
        free(stream);
    }
    return res;
}

size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    size_t total = size * nmemb;
    int res = read(stream->fd, ptr, total);
    if (res < 0) {
        stream->error = 1;
        return 0;
    }
    if (res == 0) stream->eof = 1;
    return res / size;
}

size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream) {
    size_t total = size * nmemb;
    int res = write(stream->fd, ptr, total);
    if (res < 0) {
        stream->error = 1;
        return 0;
    }
    return res / size;
}

int fseek(FILE *stream, long offset, int whence) {
    int res = lseek(stream->fd, offset, whence);
    if (res < 0) return -1;
    stream->eof = 0; // Reset EOF
    return 0;
}

long ftell(FILE *stream) {
    return lseek(stream->fd, 0, SEEK_CUR);
}

int fflush(FILE *stream) {
    return 0;
}

// Minimal Printf
static void print_decimal(FILE *stream, int val) {
    if (val < 0) {
        fputc('-', stream);
        val = -val;
    }
    if (val == 0) {
        fputc('0', stream);
        return;
    }
    char buf[32];
    int i = 0;
    while (val > 0) {
        buf[i++] = (val % 10) + '0';
        val /= 10;
    }
    while (i > 0) {
        fputc(buf[--i], stream);
    }
}

static void print_string(FILE *stream, char *s) {
    if (!s) s = "(null)";
    while(*s) fputc(*s++, stream);
}

static void print_hex(FILE *stream, unsigned int val) {
    char buf[32];
    int i = 0;
    if (val == 0) {
         fputc('0', stream);
         return;
    }
    while (val > 0) {
        int d = val % 16;
        if (d < 10) buf[i++] = d + '0';
        else buf[i++] = d - 10 + 'a';
        val /= 16;
    }
    while(i>0) fputc(buf[--i], stream);
}

int vfprintf(FILE *stream, const char *format, va_list ap) {
    int count = 0;
    while (*format) {
        if (*format == '%') {
            format++;
            switch (*format) {
                case 'd': {
                    int val = va_arg(ap, int);
                    print_decimal(stream, val);
                    break;
                }
                case 's': {
                    char *val = va_arg(ap, char*);
                    print_string(stream, val);
                    break;
                }
                case 'c': {
                    int val = va_arg(ap, int);
                    fputc(val, stream);
                    break;
                }
                case 'x': {
                    unsigned int val = va_arg(ap, unsigned int);
                    print_hex(stream, val);
                    break;
                }
                default:
                    fputc(*format, stream);
                    break;
            }
        } else {
            fputc(*format, stream);
        }
        format++;
    }
    return count;
}

int fprintf(FILE *stream, const char *format, ...) {
    va_list ap;
    va_start(ap, format);
    int res = vfprintf(stream, format, ap);
    va_end(ap);
    return res;
}

int printf(const char *format, ...) {
    va_list ap;
    va_start(ap, format);
    int res = vfprintf(stdout, format, ap);
    va_end(ap);
    return res;
}

int sprintf(char *str, const char *format, ...) {
    // Stub
    return 0;
}

int snprintf(char *str, size_t size, const char *format, ...) {
    // Stub
    return 0;
}

int fputc(int c, FILE *stream) {
    char ch = (char)c;
    write(stream->fd, &ch, 1);
    return c;
}

int fputs(const char *s, FILE *stream) {
    int len = strlen(s);
    write(stream->fd, s, len);
    return len;
}

int putc(int c, FILE *stream) {
    return fputc(c, stream);
}

int putchar(int c) {
    return fputc(c, stdout);
}

int puts(const char *s) {
    fputs(s, stdout);
    fputc('\n', stdout);
    return 0;
}

FILE *fdopen(int fd, const char *mode) {
    if (fd < 0) return NULL;
    FILE *f = (FILE*)malloc(sizeof(FILE));
    f->fd = fd;
    f->error = 0;
    f->eof = 0;
    return f;
}

FILE *freopen(const char *pathname, const char *mode, FILE *stream) {
    if (!stream) return fopen(pathname, mode);
    fclose(stream); // Close old fd
    int flags = 0;
    if (strchr(mode, 'w')) flags = 1;
    int fd = open(pathname, flags);
    if (fd < 0) return NULL;
    stream->fd = fd;
    stream->error = 0;
    stream->eof = 0;
    return stream;
}

int vsnprintf(char *str, size_t size, const char *format, va_list ap) {
    // Minimal implementation that only supports simple strings/ints
    // and DOES NOT respect size limit safely (be careful)
    // Actually, let's respect size.
    
    char *out = str;
    char *end = str + size - 1;
    
    while (*format && out < end) {
        if (*format == '%') {
            format++;
            switch (*format) {
                case 'd': {
                    int val = va_arg(ap, int);
                    // Convert and copy
                     if (val == 0) { if (out < end) *out++ = '0'; }
                     else {
                         char buf[32]; int i=0;
                         if (val < 0) { if (out < end) *out++ = '-'; val = -val; }
                         while (val) { buf[i++] = (val%10)+'0'; val/=10; }
                         while (i && out < end) *out++ = buf[--i];
                     }
                    break;
                }
                case 's': {
                    char *s = va_arg(ap, char*);
                    if (!s) s="(null)";
                    while (*s && out < end) *out++ = *s++;
                    break;
                }
                case 'c': {
                    int c = va_arg(ap, int);
                    if (out < end) *out++ = c;
                    break;
                }
                default:
                    if (out < end) *out++ = *format;
            }
        } else {
            if (out < end) *out++ = *format;
        }
        format++;
    }
    *out = 0;
    return out - str;
}

int fgetc(FILE *stream) {
    unsigned char c;
    if (fread(&c, 1, 1, stream) == 1) return c;
    return EOF; // EOF
}

int remove(const char *pathname) {
    return unlink(pathname);
}
