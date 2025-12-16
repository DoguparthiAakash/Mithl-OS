#include "memory.h"
#include "stddef.h"
// Dynamic Heap Allocator Implementation

// Memory Header Structure
typedef struct memory_header {
    size_t size;        // Total size of block (including header)
    uint8_t used;       // 1 if used, 0 if free
    struct memory_header *next; // Next block in list
} memory_header_t;

// 8MB is enough for 1024x768 framebuffer (3MB) + Kernel Heap (5MB)
#undef MEMORY_POOL_SIZE
#define MEMORY_POOL_SIZE (8 * 1024 * 1024) 

// Align allocations to 4 bytes
#define ALIGN(x) (((x) + 3) & ~3)
#define HEADER_SIZE (ALIGN(sizeof(memory_header_t)))

// Static pool
static uint8_t memory_pool[MEMORY_POOL_SIZE];
static memory_header_t *heap_head = NULL;
// Track usage
static size_t memory_used_bytes = 0;

// Initialize memory management
void memory_init(void)
{
    // Initialize the first block covering the entire pool
    heap_head = (memory_header_t *)memory_pool;
    heap_head->size = MEMORY_POOL_SIZE;
    heap_head->used = 0;
    heap_head->next = NULL;
    
    memory_used_bytes = 0;
}

// Coalesce free blocks to reduce fragmentation
// Scan entire list - O(N). Only call when necessary.
static void memory_coalesce_full(void) {
    memory_header_t *curr = heap_head;
    while (curr && curr->next) {
        if (!curr->used && !curr->next->used) {
            // Merge current and next
            curr->size += curr->next->size;
            curr->next = curr->next->next;
            // Don't advance `curr`, try to merge with the new next
        } else {
            curr = curr->next;
        }
    }
}

// Allocate memory (Best Fit or First Fit)
void *memory_alloc(size_t size)
{
    if (size == 0) return NULL;
    
    // Align to 4 bytes to ensure structural alignment
    size_t aligned_size = ALIGN(size);
    size_t total_req = aligned_size + HEADER_SIZE;
    
    memory_header_t *curr = heap_head;
    memory_header_t *best_fit = NULL;
    
    // 1. Quick Scan (First Fit) - Optimistic
    while (curr) {
        if (!curr->used && curr->size >= total_req) {
            best_fit = curr;
            break;
        }
        curr = curr->next;
    }
    
    // 2. If check failed, try Full Coalesce and Retry
    if (!best_fit) {
        memory_coalesce_full();
        curr = heap_head;
        while (curr) {
            if (!curr->used && curr->size >= total_req) {
                best_fit = curr;
                break;
            }
            curr = curr->next;
        }
    }
    
    if (!best_fit) return NULL; // OOM
    
    // Split block if large enough
    if (best_fit->size >= total_req + HEADER_SIZE + 32) { // Increase threshold to avoid tiny frags
        memory_header_t *new_block = (memory_header_t *)((uint8_t *)best_fit + total_req);
        
        new_block->size = best_fit->size - total_req;
        new_block->used = 0;
        new_block->next = best_fit->next;
        
        best_fit->size = total_req;
        best_fit->next = new_block;
    }
    
    best_fit->used = 1;
    memory_used_bytes += best_fit->size;
    
    // Zero memory for safety
    uint8_t *ptr = (uint8_t *)best_fit + HEADER_SIZE;
    for (size_t i=0; i<aligned_size; i++) ptr[i] = 0;
    
    return (void *)ptr;
}

// Free memory
void memory_free(void *ptr)
{
    if (!ptr) return;
    
    // Get header
    memory_header_t *header = (memory_header_t *)((uint8_t *)ptr - HEADER_SIZE);
    
    // Sanity check (bounds)
    if ((uint8_t*)header < memory_pool || (uint8_t*)header >= memory_pool + MEMORY_POOL_SIZE) {
        return; // Invalid pointer
    }
    
    if (header->used == 0) return; // Double free
    
    header->used = 0;
    memory_used_bytes -= header->size;
    
    // Fast Coalesce: Only merge with NEXT block if it exists and is free
    // This is O(1)
    if (header->next && !header->next->used) {
        header->size += header->next->size;
        header->next = header->next->next;
    }
    
    // Note: We cannot easily coalesce with PREV block in a singly linked list without scan.
    // Defer full coalescing to allocation time.
}

// Get used memory size
size_t memory_get_used(void)
{
    return memory_used_bytes;
}

// Get total memory size
size_t memory_get_total(void)
{
    return MEMORY_POOL_SIZE;
}

// Dump memory information (Stub)
void memory_dump_info(void)
{
}

// Input initialization
void input_init(void)
{
    // Initialize input subsystem
}

// Check if input is available
int input_available(void)
{
    return 0; // Stub implementation
}

// Keyboard event functions
static key_event_queue_t key_queue = {0};

int keyboard_event_ready(void)
{
    return key_queue.count > 0;
}

key_event_t *receive_key_event(void)
{
    if (key_queue.count == 0)
        return NULL;

    key_event_t *event = &key_queue.events[key_queue.tail];
    key_queue.tail = (key_queue.tail + 1) % MAX_KEY_EVENTS;
    key_queue.count--;

    return event;
}

void input_add_key_event(uint16_t keycode, uint8_t action)
{
    if (key_queue.count >= MAX_KEY_EVENTS)
        return; // Queue full

    key_event_t *event = &key_queue.events[key_queue.head];
    event->keycode = keycode;
    // event->action = (key_action_t)action; // If cast needed
    event->timestamp = 0; // TODO: Real timestamp
    
    key_queue.head = (key_queue.head + 1) % MAX_KEY_EVENTS;
    key_queue.count++;
}

void free_key_event(key_event_t *event)
{
    (void)event; // Avoid unused parameter warning
    // Nothing to do - events are statically allocated
}

// Mouse event functions
static mouse_event_queue_t mouse_queue = {0};

int mouse_event_ready(void)
{
    return mouse_queue.count > 0;
}

mouse_event_t *receive_mouse_event(void)
{
    if (mouse_queue.count == 0)
        return NULL;

    mouse_event_t *event = &mouse_queue.events[mouse_queue.tail];
    mouse_queue.tail = (mouse_queue.tail + 1) % MAX_MOUSE_EVENTS;
    mouse_queue.count--;

    return event;
}

void input_add_mouse_event(mouse_event_t *event_data)
{
    if (mouse_queue.count >= MAX_MOUSE_EVENTS)
        return;

    mouse_event_t *event = &mouse_queue.events[mouse_queue.head];
    *event = *event_data;
    
    mouse_queue.head = (mouse_queue.head + 1) % MAX_MOUSE_EVENTS;
    mouse_queue.count++;
}

void free_mouse_event(mouse_event_t *event)
{
    (void)event; // Avoid unused parameter warning
    // Nothing to do - events are statically allocated
}

// memcmp moved to string.c