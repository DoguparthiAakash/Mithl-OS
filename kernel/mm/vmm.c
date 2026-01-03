#include "mm/vmm.h"
#include "mm/pmm.h"
#include "boot_info.h"
#include "string.h"
#include "console.h"
#include "ports.h"

#include "spinlock.h"

// The Kernel's Page Directory
pd_entry_t* kernel_page_directory = 0;
static lock_t vmm_lock;

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

static inline uint32_t vmm_get_cr3() {
    uint32_t cr3;
    asm volatile("mov %%cr3, %0" : "=r"(cr3));
    return cr3;
}

// Map a single page
int vmm_map_page(pd_entry_t* pd, void* phys, void* virt) {
    uint32_t flags = spinlock_acquire_irqsave(&vmm_lock);

    // If PD is provided, use it. Otherwise use current CR3.
    pd_entry_t* page_directory = pd;
    if (!page_directory) {
        page_directory = (pd_entry_t*)vmm_get_cr3();
        if (!page_directory) page_directory = kernel_page_directory;
    }
    if (!page_directory) {
        spinlock_release_irqrestore(&vmm_lock, flags);
        return 0; // Too early
    }
    
    uint32_t pd_index = (uint32_t)virt >> 22;
    uint32_t pt_index = ((uint32_t)virt >> 12) & 0x03FF;
    
    pd_entry_t* pd_entry = &page_directory[pd_index];
    
    // Check if Page Table exists
    if ((*pd_entry & I86_PDE_PRESENT) != I86_PDE_PRESENT) {
        // Allocate new Page Table
        // Drop lock while calling PMM to allow interrupts? 
        // NO. PMM is fast and irq-safe. We keep VMM lock to prevent race on *pd_entry.
        
        void* new_pt_phys = pmm_alloc_block();
        if (!new_pt_phys) {
            spinlock_release_irqrestore(&vmm_lock, flags);
            return 0; // OOM
        }
        
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
    
    spinlock_release_irqrestore(&vmm_lock, flags);
    return 1;
}



void vmm_map_framebuffer(boot_info_t* boot_info) {
    if (boot_info->framebuffer.addr != 0) {
       // Check for 64-bit address overflow
       if (boot_info->framebuffer.addr > 0xFFFFFFFF) {
           serial_write("[VMM] CRITICAL: Framebuffer is > 4GB! (");
           // print top 32 bits?
           serial_write(") Truncating/Failing.\n");
       }
       
       uint32_t fb_addr = (uint32_t)boot_info->framebuffer.addr;
       uint32_t fb_size = boot_info->framebuffer.pitch * boot_info->framebuffer.height;
       
       serial_write("[VMM] Mapping Framebuffer at: 0x");
       // Simple Hex print
        char h[] = "0123456789ABCDEF";
        for(int i=28; i>=0; i-=4) {
            char c = h[(fb_addr >> i) & 0xF];
            // serial_putc
            while ((inb(0x3F8 + 5) & 0x20) == 0);
            outb(0x3F8, c);
        }
       serial_write("\n");

       // Align size to page boundary
       if (fb_size % PAGE_SIZE) fb_size += PAGE_SIZE; // Rough roundup
       
       for (uint32_t offset = 0; offset < fb_size; offset += PAGE_SIZE) {
           vmm_map_page(kernel_page_directory, (void*)(fb_addr + offset), (void*)(fb_addr + offset));
       }
   }
}

void vmm_init(boot_info_t* boot_info) {
    spinlock_init(vmm_lock);
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
        vmm_map_page(kernel_page_directory, (void*)i, (void*)i);
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

// Clone a Page Directory
// For Fork: copy user-space mappings (deep copy pages), link kernel mappings.
pd_entry_t* vmm_clone_directory(pd_entry_t* src) {
    // 1. Alloc new PD
    pd_entry_t* new_pd = (pd_entry_t*)pmm_alloc_block();
    if (!new_pd) return 0;
    
    // 2. Clear
    memset(new_pd, 0, PMM_PAGE_SIZE);
    
    // 3. Link Kernel Mappings
    // Kernel is 0-128MB (Identity).
    // In our simplified OS, we rely on the fact that Kernel PD is static for kernel space.
    // We can copy entries from `kernel_page_directory`.
    
    // 1024 PDEs.
    // Kernel: 0 -> 128MB. Each PDE covers 4MB.
    // 128MB / 4MB = 32 entries.
    // Copies first 32 entries (Identity Map)
    for (int i=0; i<1024; i++) {
        // If it's a Kernel entry (present in kernel_page_directory), link it.
        // Simplified Logic: Copy EVERYTHING from kernel_page_directory initially.
        // But for User pages in Src (if fork), we need Deep Copy.
        
        if (src[i] & I86_PDE_PRESENT) {
            // Check if it's Kernel Space?
            // Convention: Kernel < 0xC0000000? NO, we are Identity Mapped 0-128MB.
            // Helper: Is this range Kernel?
            // i=0..32 is Kernel (Low Identity).
            // i=256+ (1GB+) might be other stuff.
            
            // For now: Link everything.
            // TODO: DEEP COPY for User Pages implementation later.
            // This is "Spawn Thread" style cloning (Shared Memory).
            // For true Fork, we need to inspect user capability bit?
            // Yes, I86_PDE_USER.
            
            if (src[i] & I86_PDE_USER) {
                 // User Page Table. We need to clone the TABLE, and clone the PAGES.
                 // This is complex Deep Copy logic.
                 // IMPLEMENTATION DEFERRED for Fork Step.
                 // For now, Shared Memory (Threads).
                 new_pd[i] = src[i]; 
            } else {
                 // Kernel Page Table - Link Shared.
                 new_pd[i] = src[i];
            }
        }
    }
    
    // Self-Recursion? No, we don't use recursive mapping here yet.
    
    return new_pd;
}

void vmm_free_directory(pd_entry_t* pd) {
    if (!pd) return;
    // Iterate and free User Page Tables?
    pmm_free_block(pd);
}

void vmm_switch_pd(pd_entry_t* pd) {
    asm volatile("mov %0, %%cr3" :: "r"(pd));
}
