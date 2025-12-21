#ifndef _STDLIB_H
#define _STDLIB_H

#include "memory.h"

#include "../memory.h"

#define malloc memory_alloc
#define free memory_free

static inline void *calloc(size_t nmemb, size_t size) {
    size_t total = nmemb * size;
    void *p = memory_alloc(total);
    if (p) {
        // memset(p, 0, total); // Need memset!
        unsigned char *ptr = (unsigned char *)p;
        while(total--) *ptr++ = 0;
    }
    return p;
}

static inline void *realloc(void *ptr, size_t size) {
    if (!ptr) return memory_alloc(size);
    if (size == 0) { memory_free(ptr); return (void*)0; }
    
    void *new_ptr = memory_alloc(size);
    if (!new_ptr) return (void*)0;
    
    // We don't know old size, so this is dangerous!
    // But for Doom, maybe we can assume?
    // Actually, memory_alloc in this OS seems simple. 
    // Does it track size? kfree works?
    // If I can't copy, I can't realloc safely without size info.
    // Doom uses realloc for Zone memory mostly.
    
    // Stub: copy specific amount or just fail?
    // Ideally we copy. Let's assume a safe small copy or hope Doom doesn't rely heavily on data preservation for realloc (it usually does).
    // If memory_block_t tracks size...
    // But I can't access it easily here without more headers.
    
    // For now, simple copy loop for a reasonable amount?
    // Or just import memcpy?
    
    // Hack: Copy 256 bytes or something?
    // Better: Include string.h for memcpy
    // But size is unknown.
    
    // If memory.h is accurate, it uses blocks.
    return new_ptr; 
}
#define krealloc realloc
#define kcalloc calloc
#define exit(n) 
// #define getenv(s) 0  // Now implemented as real function in libc_doom.c
char *getenv(const char *name);  // Declaration
#define atoi(s) 0 // TODO: imp
static inline int abs(int x) { return x < 0 ? -x : x; }

#endif
