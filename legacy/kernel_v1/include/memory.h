#ifndef MEMORY_H
#define MEMORY_H
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Define exact-width types for freestanding environment
typedef unsigned char uint8_t;
typedef signed char int8_t;
typedef unsigned short uint16_t;
typedef signed short int16_t;
typedef unsigned int uint32_t;
typedef signed int int32_t;
typedef unsigned long long uint64_t;
typedef signed long long int64_t;

#define NULL ((void *)0)
// Memory configuration - adjust sizes as needed
#define MEMORY_POOL_SIZE 65536 // 64KB total pool
#define MEMORY_BLOCK_SIZE 256  // 256 byte blocks
#define MAX_KEY_EVENTS 32      // Keyboard event queue size
#define MAX_MOUSE_EVENTS 32    // Mouse event queue size
// Memory block structure
typedef struct memory_block
{
    uint8_t data[MEMORY_BLOCK_SIZE];
    uint8_t used;
    struct memory_block *next;
} memory_block_t;
// Memory pool structure
typedef struct
{
    uint8_t pool[MEMORY_POOL_SIZE];
    memory_block_t *free_blocks;
    size_t total_blocks;
    size_t used_blocks;
} memory_pool_t;
#include "input.h"
// Event queue structures
typedef struct
{
    key_event_t events[MAX_KEY_EVENTS];
    uint16_t head;
    uint16_t tail;
    uint16_t count;
} key_event_queue_t;
typedef struct
{
    mouse_event_t events[MAX_MOUSE_EVENTS];
    uint16_t head;
    uint16_t tail;
    uint16_t count;
} mouse_event_queue_t;
// Core memory functions
void memory_init(void);
void *memory_alloc(size_t size);
void *memory_realloc(void *ptr, size_t size);
void memory_free(void *ptr);
size_t memory_get_used(void);
size_t memory_get_total(void);
void memory_dump_info(void);
// Input event API
void input_init(void);
int input_available(void);
// Keyboard functions
int keyboard_event_ready(void);
key_event_t *receive_key_event(void);
void free_key_event(key_event_t *event);
// Mouse functions
int mouse_event_ready(void);
mouse_event_t *receive_mouse_event(void);
void free_mouse_event(mouse_event_t *event);
// Memory utilities
int memcmp(const void *ptr1, const void *ptr2, size_t count);

#ifdef __cplusplus
}
#endif

#endif // MEMORY_H