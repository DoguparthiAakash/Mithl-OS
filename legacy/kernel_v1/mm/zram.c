#include "zram.h"
#include "memory.h"
#include "string.h"
#include "console.h"

// Simple RLE Compression for Kernel Mode
// Format: [Count: 1 byte][Value: 1 byte] ...
// If Count == 0, next byte is literal run length? No, keep it simple.
// Max run 255.

#define PAGE_SIZE 4096

static zram_stats_t stats = {0};

typedef struct zram_slot {
    uint32_t size;
    uint8_t *data;
    struct zram_slot *next;
    uint32_t id;
} zram_slot_t;

static zram_slot_t *slots = NULL; // Simple linked list for now (inefficient lookup but works)
static uint32_t next_id = 1;

void zram_init(void) {
    console_write("[zRAM] Initialized. Using RLE compression.\n");
}

// Simple RLE Compressor
// Returns compressed size
static uint32_t compress_rle(const uint8_t *src, uint8_t *dst) {
    uint32_t src_pos = 0;
    uint32_t dst_pos = 0;
    
    while (src_pos < PAGE_SIZE) {
        uint8_t val = src[src_pos];
        uint8_t count = 1;
        
        while (src_pos + count < PAGE_SIZE && 
               src[src_pos + count] == val && 
               count < 255) {
            count++;
        }
        
        // Check for buffer overflow
        if (dst_pos + 2 >= PAGE_SIZE) return 0; // Expanded! Abort.
        
        dst[dst_pos++] = count;
        dst[dst_pos++] = val;
        
        src_pos += count;
    }
    
    return dst_pos;
}

static void decompress_rle(const uint8_t *src, uint32_t src_len, uint8_t *dst) {
    uint32_t src_pos = 0;
    uint32_t dst_pos = 0;
    
    while (src_pos < src_len && dst_pos < PAGE_SIZE) {
        uint8_t count = src[src_pos++];
        uint8_t val = src[src_pos++];
        
        for (int i = 0; i < count; i++) {
            if (dst_pos < PAGE_SIZE) {
                dst[dst_pos++] = val;
            }
        }
    }
    // Zero fill remaining if any? RLE should cover it.
    if (dst_pos < PAGE_SIZE) {
        memset(dst + dst_pos, 0, PAGE_SIZE - dst_pos);
    }
}

uint32_t zram_store_page(void *page_data) {
    // 1. Compress
    uint8_t temp_buf[PAGE_SIZE];
    uint32_t c_size = compress_rle((uint8_t*)page_data, temp_buf);
    
    // If compression failed or didn't save space (conservative > 70%)
    if (c_size == 0 || c_size > (PAGE_SIZE * 3 / 4)) {
        // Store uncompressed? For now, just fail to swap if not compressible?
        // Or store raw. Let's store raw if RLE sucks.
        // We'll mark with size=0 or similar logic, but for simplicity let's require compression.
        // Actually, for "swap", we MUST store it.
        // If RLE fails, just copy raw.
        if (c_size == 0) {
            c_size = PAGE_SIZE;
            memcpy(temp_buf, page_data, PAGE_SIZE);
        }
    }
    
    // 2. Allocate Slot
    zram_slot_t *slot = (zram_slot_t*)memory_alloc(sizeof(zram_slot_t));
    if (!slot) return 0;
    
    slot->data = (uint8_t*)memory_alloc(c_size);
    if (!slot->data) {
        memory_free(slot);
        return 0;
    }
    
    memcpy(slot->data, temp_buf, c_size);
    slot->size = c_size;
    slot->id = next_id++;
    
    // 3. Link
    slot->next = slots;
    slots = slot;
    
    // Stats
    stats.original_size += PAGE_SIZE;
    stats.compressed_size += c_size;
    stats.pages_stored++;
    stats.pages_written++;
    
    return slot->id;
}

int zram_read_page(uint32_t handle, void *out_buffer) {
    zram_slot_t *curr = slots;
    while (curr) {
        if (curr->id == handle) {
            // Found
            if (curr->size == PAGE_SIZE) {
                memcpy(out_buffer, curr->data, PAGE_SIZE);
            } else {
                decompress_rle(curr->data, curr->size, (uint8_t*)out_buffer);
            }
            stats.pages_read++;
            return 1;
        }
        curr = curr->next;
    }
    return 0;
}

void zram_free_page(uint32_t handle) {
    zram_slot_t *curr = slots;
    zram_slot_t *prev = NULL;
    
    while (curr) {
        if (curr->id == handle) {
            // Unlink
            if (prev) prev->next = curr->next;
            else slots = curr->next;
            
            // Free
            stats.original_size -= PAGE_SIZE;
            stats.compressed_size -= curr->size;
            stats.pages_stored--;
            
            memory_free(curr->data);
            memory_free(curr);
            return;
        }
        prev = curr;
        curr = curr->next;
    }
}

void zram_get_stats(zram_stats_t *out_stats) {
    *out_stats = stats;
}
