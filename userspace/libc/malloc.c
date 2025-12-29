#include <stdlib.h>
#include <unistd.h>
#include <string.h>

// Simple Linked List Allocator
// Header for each block
typedef struct header {
    size_t size;
    struct header *next;
    int free;
} header_t;

static header_t *head = NULL;
static header_t *tail = NULL;

extern int sys_brk(void *addr); // Wrapper needed in syscall.asm/c

// We need a way to call SYS_BRK. 
// "syscall_1(SYS_BRK, new_brk)"
// If arg is 0, returns current brk.
extern int syscall_1(int num, int arg1);
#define SYS_BRK 45

void *sbrk(intptr_t increment) {
    uint32_t current_brk = syscall_1(SYS_BRK, 0);
    if (current_brk == 0) return (void*)-1;
    
    if (increment == 0) return (void*)current_brk;
    
    uint32_t new_brk = current_brk + increment;
    uint32_t real_brk = syscall_1(SYS_BRK, new_brk);
    
    if (real_brk == new_brk) return (void*)current_brk;
    else return (void*)-1; // Failed
}

void *malloc(size_t size) {
    if (size == 0) return NULL;
    
    // Align size
    size = (size + 7) & ~7; // 8-byte align
    
    header_t *curr = head;
    while(curr) {
        if (curr->free && curr->size >= size) {
            curr->free = 0;
            return (void*)(curr + 1);
        }
        curr = curr->next;
    }
    
    // No free block, request from OS
    size_t total_size = sizeof(header_t) + size;
    void *block = sbrk(total_size);
    if (block == (void*)-1) return NULL;
    
    header_t *header = (header_t*)block;
    header->size = size;
    header->free = 0;
    header->next = NULL;
    
    if (!head) head = header;
    if (tail) tail->next = header;
    tail = header;
    
    return (void*)(header + 1);
}

void free(void *ptr) {
    if (!ptr) return;
    
    header_t *header = (header_t*)ptr - 1;
    header->free = 1;
    
    // Coalesce? Not implemented for simplicity in V1
}

void *calloc(size_t nmemb, size_t size) {
    size_t total = nmemb * size;
    void *p = malloc(total);
    if (p) memset(p, 0, total);
    return p;
}

void *realloc(void *ptr, size_t size) {
    if (!ptr) return malloc(size);
    if (size == 0) {
        free(ptr);
        return NULL;
    }
    
    header_t *header = (header_t*)ptr - 1;
    if (header->size >= size) return ptr;
    
    void *new_ptr = malloc(size);
    if (new_ptr) {
        memcpy(new_ptr, ptr, header->size);
        free(ptr);
    }
    return new_ptr;
}
