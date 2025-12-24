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

typedef struct {
    uint32_t mod_start;
    uint32_t mod_end;
    char* string; // Cmdline / Name
    uint32_t reserved;
} boot_module_t;

// Unified Boot Info Structure
typedef struct {
    // Memory Map
    uint32_t mmap_count;
    boot_mmap_entry_t* mmap_entries;

    // Framebuffer
    boot_framebuffer_t framebuffer;
    
#define MAX_BOOT_MODULES 10

    // Modules (Initrd, etc.)
    uint32_t mod_count;
    boot_module_t modules[MAX_BOOT_MODULES];
    
    // Boot Loader Name
    const char* bootloader_name;
    
    // Command Line
    const char* cmdline;

    // Acpi (RSDP)
    uintptr_t acpi_rsdp;
} boot_info_t;

#endif
