#include "mm/pmm.h"
#include "boot_info.h"
#include <stdint.h>
#include "string.h"
#include "console.h"

// Fix for missing uintptr_t in some freestanding envs
#ifndef _UINTPTR_T_DEFINED
typedef uint32_t uintptr_t;
#define _UINTPTR_T_DEFINED
#endif

// 4GB Max RAM support (for now)
// Bitmap size: 4GB / 4KB / 8 bits = 131072 bytes (128KB bitmap)
// We can allocate this statically in .bss
#define PMM_MAX_FRAMES (1024 * 1024) 
static uint32_t pmm_bitmap[PMM_MAX_FRAMES / 32];
static size_t pmm_total_blocks = 0;
static size_t pmm_used_blocks = 0;

// Helper: Set bit
static void pmm_set_frame(uint32_t frame_idx) {
    pmm_bitmap[frame_idx / 32] |= (1 << (frame_idx % 32));
}

// Helper: Clear bit
static void pmm_unset_frame(uint32_t frame_idx) {
    pmm_bitmap[frame_idx / 32] &= ~(1 << (frame_idx % 32));
}

// Helper: Check bit
static int pmm_test_frame(uint32_t frame_idx) {
    return (pmm_bitmap[frame_idx / 32] & (1 << (frame_idx % 32)));
}

// Find first free frame
static int pmm_first_free_frame() {
    for (size_t i = 0; i < PMM_MAX_FRAMES / 32; i++) {
        if (pmm_bitmap[i] != 0xFFFFFFFF) {
            // At least one bit is free here
            for (int j = 0; j < 32; j++) {
                int bit = 1 << j;
                if (!(pmm_bitmap[i] & bit)) {
                    return i * 32 + j;
                }
            }
        }
    }
    return -1;
}

void pmm_init(boot_info_t* boot_info) {
    // 1. Mark ALL memory as used by default (safer)
    memset(pmm_bitmap, 0xFF, sizeof(pmm_bitmap));
    pmm_used_blocks = PMM_MAX_FRAMES;
    pmm_total_blocks = PMM_MAX_FRAMES;

    // 2. Parse Unified Memory Map to identify VALID RAM
    // and mark those regions as free (unset bit)
    for (uint32_t i = 0; i < boot_info->mmap_count; i++) {
        boot_mmap_entry_t* entry = &boot_info->mmap_entries[i];

        if (entry->type == BOOT_MMAP_AVAILABLE) {
            // Initialize this region
            // Align to page boundaries
            uint64_t addr = entry->base_addr;
            uint64_t len = entry->length;
            
            // Skip low memory < 1MB to preserve BIOS/VGA/Kernel
            if (addr < 0x100000) {
                    // Use partial length if it crosses 1MB?
                    // Just skip low mem for simplicity, PMM usually manages high mem
                    // But we should free > 1MB
                    if (addr + len > 0x100000) {
                        uint64_t diff = 0x100000 - addr;
                        addr = 0x100000;
                        len -= diff;
                    } else {
                        // Entire region is low mem
                        continue;
                    }
            }
            
            
            // Debug Region
            char mbuf[64];
            // Simple hex print hack if no sprintf?
            // "Region: Base: %x Len: %x\n"
             console_write("[PMM] Region Base: ");
             // ... printing hex is hard without sprintf/itoa
             // Let's assume console_write verifies it works.
             // I'll use serial_write which is safer.
             // I don't have sprintf in pmm.c?
             // It's a kernel file, it can use kernel sprintf if available.
             // kernel.c has snprintf commented out.
             // I can just rely on the FACT that pmm_init works, 
             // and just print if F3F000 is inside range.
            
            if (addr <= 0xF3F000 && (addr + len) > 0xF3F000) {
                 console_write(" [PMM] NOTE: Module 0xF3F000 is inside this RAM region!\n");
                 serial_write(" [PMM] NOTE: Module 0xF3F000 is inside this RAM region!\n");
            }

            pmm_init_region((uint32_t)addr, (size_t)len);
        }
    }
    
    // 3. Mark Kernel Memory as Used (so we don't lease it out)
    // We need to know kernel start/end. 
    // Usually provided by linker symbols: extern int end;
    // Let's assume kernel size < 16MB for now or use specific symbol if available
    // Safer: Mark first 16MB as used.
    // Safer: Mark first 16MB as used.
    extern uint32_t _kernel_end; // Linker symbol
    uint32_t kernel_end = (uint32_t)&_kernel_end;
    
    // De-initialize (mark used) 0 to kernel_end + padding
    pmm_deinit_region(0x0, kernel_end + 1024*1024); // +1MB for safety/stack
    
    // 4. Mark Modules as Used
    for (uint32_t i = 0; i < boot_info->mod_count; i++) {
        uint32_t start = boot_info->modules[i].mod_start;
        uint32_t end = boot_info->modules[i].mod_end;
        uint32_t size = end - start;
        
        console_write("[PMM] Reserving Module: ");
        // print hex helper missing, assume debugged via result
        // serial_write("[PMM] Reserving Module\n");
        
        // Ensure we don't accidentally free it later? 
        // We already marked available RAM as free. Now we mark this specific part as used.
        pmm_deinit_region(start, size);
    }
}

void pmm_init_region(uint32_t base, size_t size) {
    uint32_t align_base = base / PMM_BLOCK_SIZE;
    size_t blocks = size / PMM_BLOCK_SIZE;
    
    for (size_t i = 0; i < blocks; i++) {
        if (pmm_test_frame(align_base + i)) {
            // It was used (default), now set to free
            pmm_unset_frame(align_base + i);
            pmm_used_blocks--;
        }
    }
}

void pmm_deinit_region(uint32_t base, size_t size) {
    uint32_t align_base = base / PMM_BLOCK_SIZE;
    size_t blocks = size / PMM_BLOCK_SIZE;
    if (size % PMM_BLOCK_SIZE) blocks++;
    
    for (size_t i = 0; i < blocks; i++) {
        if (!pmm_test_frame(align_base + i)) {
            // It was free, now set to used
            pmm_set_frame(align_base + i);
            pmm_used_blocks++;
        }
    }
}

void* pmm_alloc_block() {
    int frame = pmm_first_free_frame();
    if (frame == -1) return NULL; // OOM
    
    pmm_set_frame(frame);
    pmm_used_blocks++;
    
    uint32_t addr = frame * PMM_BLOCK_SIZE;
    return (void*)addr;
}

void pmm_free_block(void* p) {
    uint32_t addr = (uint32_t)p;
    int frame = addr / PMM_BLOCK_SIZE;
    
    if (pmm_test_frame(frame)) {
        pmm_unset_frame(frame);
        pmm_used_blocks--;
    }
}

size_t pmm_get_total_memory() {
    return pmm_total_blocks * PMM_BLOCK_SIZE; // Rough upper bound
}
size_t pmm_get_used_memory() {
    return pmm_used_blocks * PMM_BLOCK_SIZE;
}
size_t pmm_get_free_memory() {
    return (pmm_total_blocks - pmm_used_blocks) * PMM_BLOCK_SIZE;
}
