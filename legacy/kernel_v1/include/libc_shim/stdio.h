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
int printf(const char *format, ...);
int fprintf(FILE *stream, const char *format, ...);
int vfprintf(FILE *stream, const char *format, va_list ap);
int fflush(FILE *stream);

#endif
