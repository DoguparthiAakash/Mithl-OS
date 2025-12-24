#include "mm/vmm.h"
#include "mm/pmm.h"
#include "boot_info.h"
#include "string.h"
#include "console.h"

// The Kernel's Page Directory
pd_entry_t* kernel_page_directory = 0;

// ASM functions (could be inline, but good to decl)
extern void load_page_directory(unsigned int*);
extern void enable_paging();

// Inline ASM implementation for enable/load if not external
void vmm_load_pd(uint32_t* pd) {
    asm volatile("mov %0, %%cr3" :: "r"(pd));
}

void vmm_enable_paging_asm() {
    uint32_t cr0;
    asm volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000; // Bit 31: PG (Paging)
    asm volatile("mov %0, %%cr0" :: "r"(cr0));
}

// Helper to invalidate TLB for a page
static inline void vmm_flush_tlb_entry(void *addr) {
    asm volatile("invlpg (%0)" :: "r" (addr) : "memory");
}

// Map a single page
int vmm_map_page(void* phys, void* virt) {
    pd_entry_t* page_directory = kernel_page_directory;
    
    uint32_t pd_index = (uint32_t)virt >> 22;
    uint32_t pt_index = ((uint32_t)virt >> 12) & 0x03FF;
    
    pd_entry_t* pd_entry = &page_directory[pd_index];
    
    // Check if Page Table exists
    if ((*pd_entry & I86_PDE_PRESENT) != I86_PDE_PRESENT) {
        // Allocate new Page Table
        void* new_pt_phys = pmm_alloc_block();
        if (!new_pt_phys) return 0; // OOM
        
        memset(new_pt_phys, 0, PMM_PAGE_SIZE); // Clear it
        
        // Add to PD (User | Writable | Present)
        // Enable USER access (0x4) so Ring 3 can access this range if PTE permits
        *pd_entry = (uint32_t)new_pt_phys | I86_PDE_PRESENT | I86_PDE_WRITABLE | I86_PDE_USER;
    }
    
    // Get Page Table
    // Note: This only works if new_pt_phys is IDENTITY MAPPED in Kernel space.
    // Since we identity map first 128MB, and PMM allocates from low mem first, this works.
    pt_entry_t* page_table = (pt_entry_t*)((*pd_entry) & ~0xFFF);
    
    // Get entry
    pt_entry_t* pt_entry = &page_table[pt_index];
    
    // Set Entry
    *pt_entry = (uint32_t)phys | I86_PTE_PRESENT | I86_PTE_WRITABLE | I86_PTE_USER;
    
    // FLUSH TLB to ensure CPU sees the new mapping!
    vmm_flush_tlb_entry(virt);
    
    return 1;
}



void vmm_map_framebuffer(boot_info_t* boot_info) {
   if (boot_info->framebuffer.addr != 0) {
       uint32_t fb_addr = (uint32_t)boot_info->framebuffer.addr;
       uint32_t fb_size = boot_info->framebuffer.pitch * boot_info->framebuffer.height;
       
       // Align size to page boundary
       if (fb_size % PAGE_SIZE) fb_size += PAGE_SIZE; // Rough roundup
       
       serial_write("[VMM] Mapping Framebuffer at: ");
       // print hex fb_addr (TODO)
       serial_write("\n");

       for (uint32_t offset = 0; offset < fb_size; offset += PAGE_SIZE) {
           vmm_map_page((void*)(fb_addr + offset), (void*)(fb_addr + offset));
       }
   }
}

void vmm_init(boot_info_t* boot_info) {
    serial_write("[VMM] Allocating Page Directory...\n");
    // 1. Allocate Page Directory from PMM
    kernel_page_directory = (pd_entry_t*)pmm_alloc_block();
    if (!kernel_page_directory) {
        serial_write("[VMM] FATAL: Could not allocate PD!\n");
        return; 
    }
    
    // Clear PD
    memset(kernel_page_directory, 0, PMM_PAGE_SIZE);
    
    serial_write("[VMM] Identity Mapping First 128MB...\n");
    // 2. Identity Map the first 128 MB of memory
    // This covers BIOS, VGA, Kernel, and GRUB structures (Multiboot info can be > 16MB)
    // 128MB = 32 * 1024 pages
    uint32_t i = 0;
    while (i < (128 * 1024 * 1024)) { // 128 MB
        vmm_map_page((void*)i, (void*)i);
        i += PAGE_SIZE;
    }
    
    // Map VESA Framebuffer
    vmm_map_framebuffer(boot_info);
    
    serial_write("[VMM] Loading CR3...\n");
    // 3. Load CR3
    vmm_load_pd((uint32_t*)kernel_page_directory);
    
    serial_write("[VMM] Enabling Paging (CR0)...\n");
    // 4. Enable Paging
    vmm_enable_paging_asm();
}
