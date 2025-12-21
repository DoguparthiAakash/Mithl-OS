#ifndef _STDIO_H
#define _STDIO_H

#include "console.h"
#include <stdarg.h> // GCC provided

#define stderr 2
#define stdout 1
#define stdin 0

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

typedef int FILE;

extern int sprintf(char *str, const char *format, ...);
static inline int printf(const char *format, ...) { return 0; }
static inline int fprintf(FILE *stream, const char *format, ...) { return 0; }
static inline int vfprintf(FILE *stream, const char *format, va_list ap) { return 0; }
static inline int fflush(FILE *stream) { return 0; }

#endif
