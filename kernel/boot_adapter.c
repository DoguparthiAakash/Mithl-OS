#include "boot_info.h"
#include "multiboot.h"
#include "multiboot2.h"
#include <stddef.h> // for NULL

// Static buffer for unified memory map to avoid malloc usage before heap initialization
#ifndef _UINTPTR_T_DEFINED
typedef uint32_t uintptr_t;
#define _UINTPTR_T_DEFINED
#endif

#define MAX_MMAP_ENTRIES 64
static boot_mmap_entry_t mmap_buffer[MAX_MMAP_ENTRIES];

static void parse_multiboot1(multiboot_info_t* mbi, boot_info_t* info) {
    info->bootloader_name = (mbi->flags & MULTIBOOT_FLAG_LOADER) ? (char*)mbi->boot_loader_name : "Multiboot 1 Loader";
    info->cmdline = (mbi->flags & MULTIBOOT_FLAG_CMDLINE) ? (char*)mbi->cmdline : "";
    info->acpi_rsdp = 0; // MB1 doesn't usually provide ACPI directly (needs scan)

    // Parse Memory Map
    info->mmap_count = 0;
    info->mmap_entries = mmap_buffer;
    
    if (mbi->flags & MULTIBOOT_FLAG_MMAP) {
        multiboot_memory_map_t* mmap = (multiboot_memory_map_t*)mbi->mmap_addr;
        uintptr_t mmap_end = (uintptr_t)mbi->mmap_addr + mbi->mmap_length;
        
        while ((uintptr_t)mmap < mmap_end && info->mmap_count < MAX_MMAP_ENTRIES) {
            boot_mmap_entry_t* entry = &mmap_buffer[info->mmap_count];
            
            entry->base_addr = ((uint64_t)mmap->addr_high << 32) | mmap->addr;
            entry->length = ((uint64_t)mmap->len_high << 32) | mmap->len;
            entry->type = mmap->type;
            
            info->mmap_count++;
            mmap = (multiboot_memory_map_t*)((uintptr_t)mmap + mmap->size + sizeof(unsigned int));
        }
    }

    // Parse Framebuffer
    if (mbi->flags & MULTIBOOT_FLAG_FB) {
        info->framebuffer.addr = mbi->framebuffer_addr;
        info->framebuffer.width = mbi->framebuffer_width;
        info->framebuffer.height = mbi->framebuffer_height;
        info->framebuffer.pitch = mbi->framebuffer_pitch;
        info->framebuffer.bpp = mbi->framebuffer_bpp;
    } else {
        info->framebuffer.addr = 0;
    }
}

static void parse_multiboot2(unsigned long addr, boot_info_t* info) {
    // MB2 structure starts with total size (u32) then reserved (u32)
    // Tags start at offset 8
    
    // We assume 32-bit pointers for now
    multiboot_tag_t* tag;
    tag = (multiboot_tag_t*)(addr + 8);
    
    info->mmap_count = 0;
    info->mmap_entries = mmap_buffer;
    info->framebuffer.addr = 0;
    info->bootloader_name = "Multiboot 2 Loader";
    info->bootloader_name = "Multiboot 2 Loader";
    info->cmdline = "";
    info->acpi_rsdp = 0;

    while (tag->type != MULTIBOOT_TAG_TYPE_END) {
        switch (tag->type) {
            case MULTIBOOT_TAG_TYPE_CMDLINE:
                info->cmdline = ((multiboot_tag_string_t*)tag)->string;
                break;
                
            case MULTIBOOT_TAG_TYPE_BOOT_LOADER_NAME:
                info->bootloader_name = ((multiboot_tag_string_t*)tag)->string;
                break;
                
            case MULTIBOOT_TAG_TYPE_MMAP: {
                multiboot_tag_mmap_t* mmap_tag = (multiboot_tag_mmap_t*)tag;
                multiboot_mmap_entry_t* mmap = (multiboot_mmap_entry_t*)((uint8_t*)tag + sizeof(multiboot_tag_mmap_t));
                
                // Calculate number of entries
                // Tag size includes header, so subtract header size
                uint32_t entries_size = mmap_tag->size - sizeof(multiboot_tag_mmap_t);
                uint32_t count = entries_size / mmap_tag->entry_size;
                
                for (uint32_t i = 0; i < count && info->mmap_count < MAX_MMAP_ENTRIES; i++) {
                    boot_mmap_entry_t* dest = &mmap_buffer[info->mmap_count];
                    
                    // Note: MB2 mmap entry matches our unified struct (base, len, type, reserved)
                    // We can manually copy to be safe regarding packing/padding
                    dest->base_addr = mmap->addr;
                    dest->length = mmap->len;
                    dest->type = mmap->type;
                    
                    info->mmap_count++;
                    mmap = (multiboot_mmap_entry_t*)((uintptr_t)mmap + mmap_tag->entry_size);
                }
                break;
            }
            
            case MULTIBOOT_TAG_TYPE_FRAMEBUFFER: {
                multiboot_tag_framebuffer_t* fb = (multiboot_tag_framebuffer_t*)tag;
                info->framebuffer.addr = fb->addr;
                info->framebuffer.width = fb->width;
                info->framebuffer.height = fb->height;
                info->framebuffer.pitch = fb->pitch;
                info->framebuffer.bpp = fb->bpp;
                break;
            }

            case MULTIBOOT_TAG_TYPE_ACPI_OLD: {
                multiboot_tag_acpi_t *acpi = (multiboot_tag_acpi_t *)tag;
                info->acpi_rsdp = (uintptr_t)acpi->rsdp;
                break;
            }
            
            case MULTIBOOT_TAG_TYPE_ACPI_NEW: {
                multiboot_tag_acpi_t *acpi = (multiboot_tag_acpi_t *)tag;
                info->acpi_rsdp = (uintptr_t)acpi->rsdp;
                break;
            }
            
            case MULTIBOOT_TAG_TYPE_MODULE: {
                multiboot_tag_module_t *mod = (multiboot_tag_module_t *)tag;
                if (info->mod_count < MAX_BOOT_MODULES) {
                    boot_module_t* m = &info->modules[info->mod_count++];
                    m->mod_start = mod->mod_start;
                    m->mod_end = mod->mod_end;
                    m->string = (char*)mod->string;
                }
                break;
            }
             

        }
        
        // Move to next tag (must be 8-byte aligned)
        uintptr_t addr = (uintptr_t)tag;
        addr += tag->size;
        if (addr % MULTIBOOT_TAG_ALIGN != 0) {
            addr += MULTIBOOT_TAG_ALIGN - (addr % MULTIBOOT_TAG_ALIGN);
        }
        tag = (multiboot_tag_t*)addr;
    }
}

int parse_multiboot(uint32_t magic, void* addr, boot_info_t* info) {
    if (magic == MULTIBOOT_MAGIC) {
        parse_multiboot1((multiboot_info_t*)addr, info);
        return 0; // Success
    } else if (magic == MULTIBOOT2_BOOTLOADER_MAGIC) {
        parse_multiboot2((uint32_t)addr, info);
        return 0; // Success
    }
    
    return -1; // Unknown magic
}
