#ifndef _STRING_H
#define _STRING_H

#include <stdint.h>
#include <stddef.h>

// Prototypes only (implemented in string.c)
size_t strlen(const char *s);
void *memcpy(void *dest, const void *src, size_t n);
void *memset(void *s, int c, size_t n);
char *strcpy(char *dest, const char *src);
char *strncpy(char *dest, const char *src, size_t n);
char *strcat(char *dest, const char *src);
char *strstr(const char *haystack, const char *needle);
int memcmp(const void *s1, const void *s2, size_t n);
int strcmp(const char *s1, const char *s2);
int bcmp(const void *s1, const void *s2, size_t n);
int strncmp(const char *s1, const char *s2, size_t n);

#endif // _STRING_H
