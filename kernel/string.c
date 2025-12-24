// kernel/string.c
#include "string.h"

size_t strlen(const char *s)
{
    size_t len = 0;
    while (s[len])
        len++;
    return len;
}

void *memcpy(void *dest, const void *src, size_t n)
{
    // Optimized assembly memcpy
    size_t dwords = n / 4;
    size_t bytes = n % 4;
    
    // Copy 4-byte chunks
    __asm__ volatile ("rep movsl" 
                      : // No output
                      : "S"(src), "D"(dest), "c"(dwords) 
                      : "memory");
                      
    // Copy remaining bytes
    if (bytes > 0) {
        // We need to adjust pointers because rep movsl advanced them
        // Actually, logic is tricky if using inputs directly. 
        // Better to update pointers or just recalc relative offset?
        
        // Easier: Just use pointers updated by C logic or manually adjust?
        // Let's use clean standard logic:
        
        uint8_t *d = (uint8_t*)dest + (dwords * 4);
        const uint8_t *s = (const uint8_t*)src + (dwords * 4);
        while (bytes--) *d++ = *s++;
    }
    return dest;
}

void *memset(void *s, int c, size_t n)
{
    // Optimized assembly memset
    size_t dwords = n / 4;
    size_t bytes = n % 4;
    uint32_t val32 = (unsigned char)c;
    val32 |= (val32 << 8);
    val32 |= (val32 << 16);
    
    __asm__ volatile ("rep stosl"
                      : 
                      : "D"(s), "a"(val32), "c"(dwords)
                      : "memory");
                      
    if (bytes > 0) {
        uint8_t *d = (uint8_t*)s + (dwords * 4);
        while (bytes--) *d++ = (unsigned char)c;
    }
    return s;
}

char *strcpy(char *dest, const char *src)
{
    char *ret = dest;
    while ((*dest++ = *src++))
        ;
    return ret;
}


void *memmove(void *dest, const void *src, size_t n) {
    unsigned char *d = dest;
    const unsigned char *s = src;
    if (d < s) {
        while (n--) *d++ = *s++;
    } else {
        d += n;
        s += n;
        while (n--) *--d = *--s;
    }
    return dest;
}

int memcmp(const void *s1, const void *s2, size_t n)
{
    const unsigned char *p1 = s1, *p2 = s2;
    while (n--) {
        if (*p1 != *p2)
            return *p1 - *p2;
        p1++;
        p2++;
    }
    return 0;
}

int bcmp(const void *s1, const void *s2, size_t n)
{
    return memcmp(s1, s2, n);
}

char *strcat(char *dest, const char *src)
{
    char *ptr = dest + strlen(dest);
    while (*src != '\0') {
        *ptr++ = *src++;
    }
    *ptr = '\0';
    return dest;
}

int strcmp(const char *s1, const char *s2)
{
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char *)s1 - *(const unsigned char *)s2;
}

char *strncpy(char *dest, const char *src, size_t n)
{
    size_t i;
    for (i = 0; i < n && src[i] != '\0'; i++)
        dest[i] = src[i];
    for ( ; i < n; i++)
        dest[i] = '\0';
    return dest;
}

char *strstr(const char *haystack, const char *needle)
{
    if (!*needle) return (char *)haystack;
    for (; *haystack; haystack++) {
        if (*haystack == *needle) {
             const char *h = haystack;
             const char *n = needle;
             while (*h && *n && *h == *n) {
                 h++; n++;
             }
             if (!*n) return (char *)haystack;
        }
    }
    return NULL;
}

int strncmp(const char *s1, const char *s2, size_t n) {
    while (n && *s1 && (*s1 == *s2)) {
        s1++;
        s2++;
        n--;
    }
    if (n == 0) return 0;
    return *(const unsigned char *)s1 - *(const unsigned char *)s2;
}
