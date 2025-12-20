#ifndef MULTIBOOT2_H
#define MULTIBOOT2_H

#include <stdint.h>

#define MULTIBOOT2_MAGIC 0x36d76289
#define MULTIBOOT2_BOOTLOADER_MAGIC 0x36d76289

// Alignment of multiboot structures
#define MULTIBOOT_TAG_ALIGN 8

// Tag types
#define MULTIBOOT_TAG_TYPE_END 0
#define MULTIBOOT_TAG_TYPE_CMDLINE 1
#define MULTIBOOT_TAG_TYPE_BOOT_LOADER_NAME 2
#define MULTIBOOT_TAG_TYPE_MODULE 3
#define MULTIBOOT_TAG_TYPE_BASIC_MEMINFO 4
#define MULTIBOOT_TAG_TYPE_BOOTDEV 5
#define MULTIBOOT_TAG_TYPE_MMAP 6
#define MULTIBOOT_TAG_TYPE_VBE 7
#define MULTIBOOT_TAG_TYPE_FRAMEBUFFER 8
#define MULTIBOOT_TAG_TYPE_ELF_SECTIONS 9
#define MULTIBOOT_TAG_TYPE_APM 10
#define MULTIBOOT_TAG_TYPE_EFI32 11
#define MULTIBOOT_TAG_TYPE_EFI64 12
#define MULTIBOOT_TAG_TYPE_SMBIOS 13
#define MULTIBOOT_TAG_TYPE_ACPI_OLD 14
#define MULTIBOOT_TAG_TYPE_ACPI_NEW 15
#define MULTIBOOT_TAG_TYPE_NETWORK 16
#define MULTIBOOT_TAG_TYPE_EFI_MMAP 17
#define MULTIBOOT_TAG_TYPE_EFI_BS 18
#define MULTIBOOT_TAG_TYPE_EFI32_IH 19
#define MULTIBOOT_TAG_TYPE_EFI64_IH 20
#define MULTIBOOT_TAG_TYPE_LOAD_BASE_ADDR 21

// Fixed structure for tag header
typedef struct {
    uint32_t type;
    uint32_t size;
} multiboot_tag_t;

// Tag: Memory Map
typedef struct {
    uint32_t type;
    uint32_t size;
    uint32_t entry_size;
    uint32_t entry_version;
    // Entries follow
} multiboot_tag_mmap_t;

typedef struct {
    uint64_t addr;
    uint64_t len;
    uint32_t type;
    uint32_t zero;
} multiboot_mmap_entry_t;

// Tag: Framebuffer
typedef struct {
    uint32_t type;
    uint32_t size;
    uint64_t addr;
    uint32_t pitch;
    uint32_t width;
    uint32_t height;
    uint8_t bpp;
    uint8_t type_no;
    uint8_t reserved;
} multiboot_tag_framebuffer_t;

// Tag: String (Cmdline, Bootloader Name)
typedef struct {
    uint32_t type;
    uint32_t size;
    char string[0];
} multiboot_tag_string_t;

// Tag: Module
typedef struct {
    uint32_t type;
    uint32_t size;
    uint32_t mod_start;
    uint32_t mod_end;
    char string[0];
} multiboot_tag_module_t;

// Tag: ACPI
typedef struct {
    uint32_t type;
    uint32_t size;
    uint8_t rsdp[0];
} multiboot_tag_acpi_t;

#endif
