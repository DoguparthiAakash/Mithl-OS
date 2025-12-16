#include "elf.h"
#include "vfs.h"
#include "mm/vmm.h"
#include "mm/pmm.h"
#include "string.h"
#include "console.h"
#include "memory.h"

// Forward declaration
extern pd_entry_t* kernel_page_directory;

// Helper wrapper for consistently logging
static void console_log(const char *msg) {
    console_write(msg);
    serial_write(msg);
}

// Helper to check ELF Magic
static int is_valid_elf(Elf32_Ehdr *hdr) {
    if (hdr->e_ident[EI_MAG0] != ELFMAG0) return 0;
    if (hdr->e_ident[EI_MAG1] != ELFMAG1) return 0;
    if (hdr->e_ident[EI_MAG2] != ELFMAG2) return 0;
    if (hdr->e_ident[EI_MAG3] != ELFMAG3) return 0;
    return 1;
}

// Load an ELF executable from the filesystem
// Returns the Entry Point address, or 0 on failure
uint32_t elf_load_file(const char *filename) {
    console_log("[ELF] Loading file: ");
    console_log(filename);
    console_log("\n");

    // 1. Open File
    extern fs_node_t *fs_root; // From kernel.c (or ideally a get_root() function)
    fs_node_t *file = finddir_fs(fs_root, (char*)filename); // Cast const away for now
    
    if (!file) {
        console_log("[ELF] Error: File not found.\n");
        return 0;
    }

    // 2. Read ELF Header
    Elf32_Ehdr hdr;
    if (read_fs(file, 0, sizeof(Elf32_Ehdr), (uint8_t*)&hdr) != sizeof(Elf32_Ehdr)) {
        console_log("[ELF] Error: Could not read ELF header.\n");
        return 0;
    }

    // 3. Verify Magic
    if (!is_valid_elf(&hdr)) {
        console_log("[ELF] Error: Invalid ELF Magic.\n");
        return 0;
    }

    if (hdr.e_type != ET_EXEC) {
        console_log("[ELF] Error: Not an executable (ET_EXEC).\n");
        return 0;
    }

    // 4. Load Program Headers
    uint32_t ph_offset = hdr.e_phoff;
    uint32_t ph_count = hdr.e_phnum;
    uint32_t ph_size = hdr.e_phentsize;

    for (uint32_t i = 0; i < ph_count; i++) {
        Elf32_Phdr ph;
        uint32_t current_offset = ph_offset + (i * ph_size);
        
        if (read_fs(file, current_offset, sizeof(Elf32_Phdr), (uint8_t*)&ph) != sizeof(Elf32_Phdr)) {
            console_log("[ELF] Error: Could not read Program Header.\n");
            return 0;
        }

        if (ph.p_type == PT_LOAD) {
            console_log("[ELF] Loading Segment at: ");
            // print hex ph.p_vaddr
            console_log("\n");

            // Allocate Pages and Map
            // VMM maps in 4KB chunks. 
            // We need to map from p_vaddr to p_vaddr + p_memsz
            
            uint32_t start_addr = ph.p_vaddr;
            uint32_t end_addr = start_addr + ph.p_memsz;
            
            // Align to 4K
            uint32_t start_page = start_addr & 0xFFFFF000;
            uint32_t end_page = (end_addr + 0xFFF) & 0xFFFFF000;

            for (uint32_t addr = start_page; addr < end_page; addr += 4096) {
                // Check if already mapped? For now assume we own the userspace (or kernel space if simple)
                void *phys = pmm_alloc_block();
                if (!phys) {
                     console_log("[ELF] Error: OOM during segment load.\n");
                     return 0;
                }
                vmm_map_page(phys, (void*)addr);
            }

            // Copy Data from file
            // Note: vmm_map_page maps it into *current* address space (kernel directory).
            // So we can write directly to 'start_addr' IF it is writable and mapped.
            // Since we just mapped it, it should be accessible.
            
            // We need to be careful with offsets. 
            // ph.p_offset in file -> ph.p_vaddr in memory.
            // p_filesz bytes.
            
            if (ph.p_filesz > 0) {
                // Simple read directly to memory
                // WARNING: If p_vaddr is in User Mode range (e.g. 0x40000000), 
                // and we are in Kernel Mode (Ring 0), we can write to it if pages are present.
                // But check read_fs implementation if it uses a buffer or direct.
                // Our read_fs takes uint8_t* buffer.
                
                read_fs(file, ph.p_offset, ph.p_filesz, (uint8_t*)ph.p_vaddr);
            }
            
            // Zero BSS (memsz > filesz)
            if (ph.p_memsz > ph.p_filesz) {
                uint32_t bss_size = ph.p_memsz - ph.p_filesz;
                memset((void*)(ph.p_vaddr + ph.p_filesz), 0, bss_size);
            }
        }
    }

    console_log("[ELF] File loaded successfully. Entry point: ");
    // print hex hdr.e_entry
    console_log("\n");

    return hdr.e_entry;
}
