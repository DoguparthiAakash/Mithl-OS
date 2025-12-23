#ifndef PMM_H
#define PMM_H

#include <stdint.h>
#include <stddef.h>
#include "boot_info.h"

// 4KB Pages
#define PMM_PAGE_SIZE 4096
#define PMM_BLOCK_SIZE PMM_PAGE_SIZE
#define PMM_BLOCKS_PER_BYTE 8

// Function definitions
void pmm_init(boot_info_t* boot_info);

// Allocation/Deallocation of 4KB physical blocks
void* pmm_alloc_block();
void pmm_free_block(void* p);

// Region management (initialize bitmap based on GRUB map)
void pmm_init_region(uint32_t base, size_t size);
void pmm_deinit_region(uint32_t base, size_t size);

// Memory Stats
size_t pmm_get_total_memory();
size_t pmm_get_used_memory();
size_t pmm_get_free_memory();

#endif
