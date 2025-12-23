#ifndef ZRAM_H
#define ZRAM_H

#include <stddef.h>
#include <stdint.h>

// zRAM Stats
typedef struct {
    size_t original_size;
    size_t compressed_size;
    size_t pages_stored;
    size_t pages_read;
    size_t pages_written;
} zram_stats_t;

// Initialize zRAM
void zram_init(void);

// Compress and store a page (returns index/handle)
// Returns 0 on failure or OOM
uint32_t zram_store_page(void *page_data);

// Read and decompress a page
// Returns 0 on failure
int zram_read_page(uint32_t handle, void *out_buffer);

// Free a stored page
void zram_free_page(uint32_t handle);

// Get stats
void zram_get_stats(zram_stats_t *stats);

#endif // ZRAM_H
