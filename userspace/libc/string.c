#include <string.h>
#include <stdint.h>

size_t strlen(const char *s) {
    size_t len = 0;
    while(s[len]) len++;
    return len;
}

void *memcpy(void *dest, const void *src, size_t n) {
    char *d = (char*)dest;
    const char *s = (const char*)src;
    for(size_t i=0; i<n; i++) d[i] = s[i];
    return dest;
}

void *memset(void *s, int c, size_t n) {
    unsigned char *p = (unsigned char*)s;
    unsigned char val = (unsigned char)c;
    for(size_t i=0; i<n; i++) p[i] = val;
    return s;
}

int memcmp(const void *s1, const void *s2, size_t n) {
    const unsigned char *p1 = s1;
    const unsigned char *p2 = s2;
    for(size_t i=0; i<n; i++) {
        if (p1[i] != p2[i]) return p1[i] - p2[i];
    }
    return 0;
}

void *memmove(void *dest, const void *src, size_t n) {
    char *d = dest;
    const char *s = src;
    if (d < s) {
        for(size_t i=0; i<n; i++) d[i] = s[i];
    } else {
        for(size_t i=n; i>0; i--) d[i-1] = s[i-1];
    }
    return dest;
}

char *strcpy(char *dest, const char *src) {
    size_t i = 0;
    while(src[i]) {
        dest[i] = src[i];
        i++;
    }
    dest[i] = 0;
    return dest;
}

char *strncpy(char *dest, const char *src, size_t n) {
    size_t i;
    for(i=0; i<n && src[i]; i++) dest[i] = src[i];
    for( ; i<n; i++) dest[i] = 0;
    return dest;
}

char *strcat(char *dest, const char *src) {
    size_t len = strlen(dest);
    size_t i = 0;
    while(src[i]) {
        dest[len + i] = src[i];
        i++;
    }
    dest[len + i] = 0;
    return dest;
}

int strcmp(const char *s1, const char *s2) {
    while(*s1 && (*s1 == *s2)) {
        s1++; s2++;
    }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

int strncmp(const char *s1, const char *s2, size_t n) {
    while(n > 0 && *s1 && (*s1 == *s2)) {
        s1++; s2++; n--;
    }
    if (n == 0) return 0;
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

char *strdup(const char *s) {
    // Requires malloc, but string.c usually doesn't include malloc directly?
    // In small libc it might. We'll leave it for now or implement if needed.
    // TCC specifically uses it. We need it.
    // Assuming malloc is available via stdlib.h
    extern void *malloc(size_t size);
    size_t len = strlen(s) + 1;
    char *new = malloc(len);
    if (new) memcpy(new, s, len);
    return new;
}

char *strchr(const char *s, int c) {
    while(*s) {
        if (*s == (char)c) return (char*)s;
        s++;
    }
    return 0;
}

char *strrchr(const char *s, int c) {
    const char *last = 0;
    while(*s) {
        if (*s == (char)c) last = s;
        s++;
    }
    if (c == 0) return (char*)s; // Pointer to terminator
    return (char*)last;
}

char *strstr(const char *haystack, const char *needle) {
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

char *strerror(int errnum) {
    return "Unknown error";
}

char *strtok(char *str, const char *delim) {
    static char *next_token = NULL;
    if (str) next_token = str;
    if (!next_token) return NULL;
    
    // Skip delimiters
    while (*next_token && strchr(delim, *next_token)) next_token++;
    if (!*next_token) return NULL;
    
    char *start = next_token;
    // Find end
    while (*next_token && !strchr(delim, *next_token)) next_token++;
    
    if (*next_token) {
        *next_token = 0;
        next_token++;
    }
    return start;
}

char *strpbrk(const char *s, const char *accept) {
    while (*s) {
        if (strchr(accept, *s)) return (char *)s;
        s++;
    }
    return NULL;
}
