#include "memory.h"
#include "stddef.h"
#include "mm/pmm.h"
#include "mm/vmm.h"
#include "console.h"
#include "string.h"

// Dynamic Heap Allocator Implementation (Paged)

// Memory Header Structure
typedef struct memory_header {
    size_t size;        // Total size of block (including header)
    uint8_t used;       // 1 if used, 0 if free
    struct memory_header *next; // Next block in list
} memory_header_t;

// Heap Configuration
#define HEAP_START_VADDR  0x40000000 // Start Heap at 1GB
#define HEAP_INITIAL_PAGES 256       // Start with 1MB (256 * 4KB)
#define PAGE_SIZE         4096

// Global State
static memory_header_t *heap_head = NULL;
static uint32_t heap_current_end = HEAP_START_VADDR;
static size_t memory_used_bytes = 0;

// Align allocations to 4 bytes
#define ALIGN(x) (((x) + 3) & ~3)
#define HEADER_SIZE (ALIGN(sizeof(memory_header_t)))

// Forward decl
static void memory_coalesce_full(void);

// Expand the heap by 'n_pages'
static int expand_heap(uint32_t n_pages) {
    if (n_pages == 0) return 0;
    
    //console_write("[MEM] Expanding heap...\n");
    
    uint32_t new_area_start = heap_current_end;
    
    // Map pages
    for (uint32_t i = 0; i < n_pages; i++) {
        void *phys = pmm_alloc_block();
        if (!phys) {
            console_write("[MEM] CRITICAL: Out of Physical RAM during expansion!\n");
            return 0;
        }
        
        vmm_map_page(phys, (void*)heap_current_end);
        
        // Zero the page for safety
        memset((void*)heap_current_end, 0, PAGE_SIZE);
        
        heap_current_end += PAGE_SIZE;
    }
    
    // Create a new free block for this expansion
    // Ideally we merge with the last block if it was free and adjacent in memory (which it is, virtually)
    // But for simplicity, we just add it to the list or append.
    
    size_t expanded_size = n_pages * PAGE_SIZE;
    
    // We need to link this new memory into the free list.
    // If heap_head is NULL (init), this is the first block.
    // If not, we append.
    
    memory_header_t *new_block = (memory_header_t *)new_area_start;
    new_block->size = expanded_size;
    new_block->used = 0;
    new_block->next = NULL;
    
    if (!heap_head) {
        heap_head = new_block;
    } else {
        // Find tail
        memory_header_t *curr = heap_head;
        while(curr->next) curr = curr->next;
        
        // Append
        curr->next = new_block;
        
        // Attempt to merge immediately if tail was free and virtually adjacent?
        // Virtual adjacency is guaranteed by heap_current_end logic.
        // We can just rely on normal coalescing.
        memory_coalesce_full(); 
    }
    
    return 1;
}

// Initialize memory management
void memory_init(void)
{
    heap_head = NULL;
    heap_current_end = HEAP_START_VADDR;
    memory_used_bytes = 0;
    
    // Initial expansion
    if (!expand_heap(HEAP_INITIAL_PAGES)) {
        console_write("PANIC: Failed to initialize Paged Heap!\n");
        for(;;) __asm__ volatile("hlt");
    }
    
    console_write("[MEM] Paged Heap Initialized at 0x40000000\n");
}

// Coalesce free blocks to reduce fragmentation
static void memory_coalesce_full(void) {
    memory_header_t *curr = heap_head;
    while (curr && curr->next) {
        // Check if NEXT is physically/virtually adjacent? 
        // Our linked list does NOT guarantee order by address.
        // To properly coalesce adjacent memory, we should sort the list or keep it sorted.
        // BUT, our expand_heap just appends.
        // For Paged Heap, simpler is just to link via 'next' pointers. 
        // 'next' pointer does not mean "next in memory", it means next in list.
        // merging blocks only makes sense if they are adjacent in MEMORY.
        
        if (!curr->used && !curr->next->used) {
             // Are they adjacent?
             uint32_t curr_end = (uint32_t)curr + curr->size;
             if (curr_end == (uint32_t)curr->next) {
                 // Yes, merge
                 curr->size += curr->next->size;
                 curr->next = curr->next->next;
                 // Stay on curr to try merging again
             } else {
                 curr = curr->next;
             }
        } else {
            curr = curr->next;
        }
    }
}

// Allocate memory
void *memory_alloc(size_t size)
{
    if (size == 0) return NULL;
    
    size_t aligned_size = ALIGN(size);
    size_t total_req = aligned_size + HEADER_SIZE;
    
    memory_header_t *curr = heap_head;
    memory_header_t *best_fit = NULL;
    
    // 1. Scan for Free Block
    while (curr) {
        if (!curr->used && curr->size >= total_req) {
            best_fit = curr;
            break; 
        }
        curr = curr->next;
    }
    
    // 2. If no block found, EXPAND HEAP
    if (!best_fit) {
        // Calculate needed pages
        // We might need just enough for this request, or a chunk.
        // Let's alloc chunks of 1MB (256 pages) or requests size
        size_t needed_bytes = total_req;
        uint32_t needed_pages = (needed_bytes + PAGE_SIZE - 1) / PAGE_SIZE;
        if (needed_pages < 32) needed_pages = 32; // Minimum 128KB expansion
        
        if (expand_heap(needed_pages)) {
            // Retry scan (expand_heap adds to list and coalesces)
            return memory_alloc(size); // Recursion (safe, bounded by RAM)
        } else {
            return NULL; // OOM
        }
    }
    
    // 3. Setup Block (Split if necessary)
    if (best_fit->size >= total_req + HEADER_SIZE + 32) {
        memory_header_t *new_block = (memory_header_t *)((uint8_t *)best_fit + total_req);
        
        new_block->size = best_fit->size - total_req;
        new_block->used = 0;
        new_block->next = best_fit->next;
        
        best_fit->size = total_req;
        best_fit->next = new_block;
    }
    
    best_fit->used = 1;
    memory_used_bytes += best_fit->size;
    
    uint8_t *ptr = (uint8_t *)best_fit + HEADER_SIZE;
    // Valid zeroing
    for (size_t i=0; i<aligned_size; i++) ptr[i] = 0;
    
    return (void *)ptr;
}

// Free memory
void memory_free(void *ptr)
{
    if (!ptr) return;
    
    memory_header_t *header = (memory_header_t *)((uint8_t *)ptr - HEADER_SIZE);
    
    // Basic validation
    if (header->used == 0) return;
    
    header->used = 0;
    memory_used_bytes -= header->size;
    
    memory_coalesce_full();
}

size_t memory_get_used(void) { return memory_used_bytes; }
size_t memory_get_total(void) { return (heap_current_end - HEAP_START_VADDR); } // Dynamic total
void memory_dump_info(void) {}

// Input subsystem stubs (Keep existing)
void input_init(void) {}
int input_available(void) { return 0; }

// Keyboard/Mouse Queue Implementation (Keep existing)
static key_event_queue_t key_queue = {0};

int keyboard_event_ready(void) { return key_queue.count > 0; }

key_event_t *receive_key_event(void) {
    if (key_queue.count == 0) return NULL;
    key_event_t *event = &key_queue.events[key_queue.tail];
    key_queue.tail = (key_queue.tail + 1) % MAX_KEY_EVENTS;
    key_queue.count--;
    return event;
}

void input_add_key_event(uint16_t keycode, uint8_t action) {
    if (key_queue.count >= MAX_KEY_EVENTS) return;
    key_event_t *event = &key_queue.events[key_queue.head];
    event->keycode = keycode;
    event->timestamp = 0;
    key_queue.head = (key_queue.head + 1) % MAX_KEY_EVENTS;
    key_queue.count++;
}
void free_key_event(key_event_t *event) { (void)event; }

static mouse_event_queue_t mouse_queue = {0};
int mouse_event_ready(void) { return mouse_queue.count > 0; }
mouse_event_t *receive_mouse_event(void) {
    if (mouse_queue.count == 0) return NULL;
    mouse_event_t *event = &mouse_queue.events[mouse_queue.tail];
    mouse_queue.tail = (mouse_queue.tail + 1) % MAX_MOUSE_EVENTS;
    mouse_queue.count--;
    return event;
}
void input_add_mouse_event(mouse_event_t *event_data) {
    if (mouse_queue.count >= MAX_MOUSE_EVENTS) return;
    mouse_event_t *event = &mouse_queue.events[mouse_queue.head];
    *event = *event_data;
    mouse_queue.head = (mouse_queue.head + 1) % MAX_MOUSE_EVENTS;
    mouse_queue.count++;
}
void free_mouse_event(mouse_event_t *event) { (void)event; }