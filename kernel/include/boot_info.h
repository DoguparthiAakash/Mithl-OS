#ifndef BOOT_INFO_H
#define BOOT_INFO_H

#include <stdint.h>

#define BOOT_MMAP_AVAILABLE 1
#define BOOT_MMAP_RESERVED  2

// Unified Memory Map Entry
typedef struct {
    uint64_t base_addr;
    uint64_t length;
    uint32_t type;
} boot_mmap_entry_t;

// Unified Framebuffer Info
typedef struct {
    uint64_t addr;
    uint32_t width;
    uint32_t height;
    uint32_t pitch;
    uint8_t  bpp;
} boot_framebuffer_t;

// Unified Boot Info Structure
typedef struct {
    // Memory Map
    uint32_t mmap_count;
    boot_mmap_entry_t* mmap_entries;

    // Framebuffer
    boot_framebuffer_t framebuffer;
    
    // Modules (Initrd, etc.)
    uint32_t mod_count;
    void* mod_addr; // Keep raw for now, or parse if needed
    
    // Boot Loader Name
    const char* bootloader_name;
    
    // Command Line
    const char* cmdline;

    // Acpi (RSDP)
    void* acpi_rsdp;
} boot_info_t;

#endif
