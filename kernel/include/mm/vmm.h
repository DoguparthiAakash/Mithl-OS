#ifndef VMM_H
#define VMM_H

#include <stdint.h>
#include <stddef.h>

// Page Table/Directory Entry Flags
#define I86_PTE_PRESENT       0x01
#define I86_PTE_WRITABLE      0x02
#define I86_PTE_USER          0x04
#define I86_PTE_WRITETHROUGH  0x08
#define I86_PTE_NOT_CACHEABLE 0x10
#define I86_PTE_ACCESSED      0x20
#define I86_PTE_DIRTY         0x40
#define I86_PTE_PAT           0x80
#define I86_PTE_GLOBAL        0x100
#define I86_PTE_FRAME         0xFFFFF000

#define I86_PDE_PRESENT       0x01
#define I86_PDE_WRITABLE      0x02
#define I86_PDE_USER          0x04
#define I86_PDE_WRITETHROUGH  0x08
#define I86_PDE_CACHE_DISABLE 0x10
#define I86_PDE_ACCESSED      0x20
#define I86_PDE_DIRTY         0x40
#define I86_PDE_PAGE_SIZE     0x80 // 4MB page if set
#define I86_PDE_GLOBAL        0x100
#define I86_PDE_FRAME         0xFFFFF000

#define PAGES_PER_TABLE 1024
#define TABLES_PER_DIR  1024
#define PAGE_SIZE       4096

// Data Types
typedef uint32_t pt_entry_t;
typedef uint32_t pd_entry_t;

#include "boot_info.h"

// Functions
void vmm_init(boot_info_t* boot_info);
int vmm_map_page(void* phys, void* virt);
void vmm_enable_paging();

#endif
