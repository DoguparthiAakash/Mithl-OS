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
// Aux Vector capacity
#define MAX_AUX_ENTRIES 32
static Elf32_auxv_t stored_auxv[MAX_AUX_ENTRIES];
static int stored_auxc = 0;

// Internal loader that doesn't reset auxv
uint32_t elf_load_file_internal(const char *filename, uint32_t *out_base, int is_interpreter) {
    console_log("[ELF] Loading: "); console_log(filename); console_log("\n");

    fs_node_t *file = vfs_resolve_path((char*)filename);
    if (!file) { console_log("[ELF] Error: File not found.\n"); return 0; }

    Elf32_Ehdr hdr;
    if (read_fs(file, 0, sizeof(Elf32_Ehdr), (uint8_t*)&hdr) != sizeof(Elf32_Ehdr)) return 0;
    if (!is_valid_elf(&hdr)) return 0;

    // Support EXEC and DYN
    if (hdr.e_type != ET_EXEC && hdr.e_type != ET_DYN) {
        console_log("[ELF] Error: Not EXEC or DYN.\n");
        return 0;
    }

    uint32_t load_bias = 0;
    // For DYN (Position Independent), we might need to pick a base address if it's not specified.
    // Usually ld-linux.so is DYN but linked at 0. We map it to a high address (e.g. 0x40000000)?
    // For simplicity, if DYN, let's keep it at 0 + offset if headers allow, or pick an address.
    // Real ld.so is often pre-linked at 0, so we must relocate it.
    // For this V1, we assume headers p_vaddr are absolute execution addresses for ET_EXEC,
    // and for ET_DYN (like ld.so) we might need to add a base.
    
    if (is_interpreter && hdr.e_type == ET_DYN) {
        // Load interpreter at 0x40000000 (standard-ish for some systems)
        // Or just trust p_vaddr?
        // ld-2.5.so p_vaddr might be 0.
        // Let's check a typical p_vaddr.
        // Assuming we need a bias.
        load_bias = 0x40000000;
    }
    
    if (out_base) *out_base = load_bias;

    uint32_t ph_offset = hdr.e_phoff;
    uint32_t ph_count = hdr.e_phnum;
    uint32_t ph_size = hdr.e_phentsize;
    
    // Store PHDR info for the *Main Executable* (not interpreter)
    if (!is_interpreter) {
        // AT_PHDR = Base + ex_phdr execution address
        // We need to find where PHDRs are loaded in memory.
        // Often they are part of the first PT_LOAD loading the headers.
        // We will calc this.
        
        // Scan for PT_PHDR
        uint32_t phdr_vaddr = 0;
        int found_phdr = 0;
        
        // We iterate efficiently
        for (uint32_t i=0; i<ph_count; i++) {
             Elf32_Phdr ph;
             read_fs(file, ph_offset + i*ph_size, sizeof(ph), (uint8_t*)&ph);
             if (ph.p_type == PT_PHDR) {
                 phdr_vaddr = ph.p_vaddr + load_bias;
                 found_phdr = 1;
                 break;
             }
        }
        
        // Assuming headers are loaded:
        if (found_phdr) {
             stored_auxv[stored_auxc++] = (Elf32_auxv_t){AT_PHDR, .a_un.a_val = phdr_vaddr};
        } else {
             // Fallback: usually headers are at start of first load
             // We can defer this till we load segments.
             stored_auxv[stored_auxc++] = (Elf32_auxv_t){AT_PHDR, .a_un.a_val = hdr.e_phoff + load_bias}; // Wrong if not mapped
        }
        
        stored_auxv[stored_auxc++] = (Elf32_auxv_t){AT_PHNUM, .a_un.a_val = ph_count};
        stored_auxv[stored_auxc++] = (Elf32_auxv_t){AT_PHENT, .a_un.a_val = ph_size};
        stored_auxv[stored_auxc++] = (Elf32_auxv_t){AT_ENTRY, .a_un.a_val = hdr.e_entry + load_bias};
    }

    char interpreter_path[256];
    interpreter_path[0] = 0;

    for (uint32_t i = 0; i < ph_count; i++) {
        Elf32_Phdr ph;
        read_fs(file, ph_offset + (i * ph_size), sizeof(Elf32_Phdr), (uint8_t*)&ph);

        if (ph.p_type == PT_INTERP) {
             // Read interpreter path
             if (ph.p_filesz < 255) {
                read_fs(file, ph.p_offset, ph.p_filesz, (uint8_t*)interpreter_path);
                interpreter_path[ph.p_filesz] = 0;
             }
        }
        else if (ph.p_type == PT_LOAD) {
            uint32_t start_addr = ph.p_vaddr + load_bias;
            uint32_t end_addr = start_addr + ph.p_memsz;
            uint32_t start_page = start_addr & 0xFFFFF000;
            uint32_t end_page = (end_addr + 0xFFF) & 0xFFFFF000;

            for (uint32_t addr = start_page; addr < end_page; addr += 4096) {
                void *phys = pmm_alloc_block();
                if (!phys) { return 0; } // OOM
                vmm_map_page(0, phys, (void*)addr);
            }

            if (ph.p_filesz > 0) {
                read_fs(file, ph.p_offset, ph.p_filesz, (uint8_t*)(start_addr));
            }
            if (ph.p_memsz > ph.p_filesz) {
                memset((void*)(start_addr + ph.p_filesz), 0, ph.p_memsz - ph.p_filesz);
            }
        }
    }

    // If we have an interpreter and we are the main executable...
    if (interpreter_path[0] && !is_interpreter) {
         console_log("[ELF] Interpreter identified: ");
         console_log(interpreter_path);
         console_log("\n");
         
         // Load the interpreter!
         uint32_t terp_base = 0;
         uint32_t terp_entry = elf_load_file_internal(interpreter_path, &terp_base, 1);
         if (!terp_entry) return 0; // Failed to load interpreter
         
         // Add AT_BASE
         stored_auxv[stored_auxc++] = (Elf32_auxv_t){AT_BASE, .a_un.a_val = terp_base};
         
         return terp_entry;
    }

    return hdr.e_entry + load_bias;
}

// Wrapper to expose Aux Vector
uint32_t elf_load_file(const char *filename) {
    stored_auxc = 0; // Reset
    return elf_load_file_internal(filename, NULL, 0);
}

// Accessor for Process Manager
int elf_get_auxv(Elf32_auxv_t *buf, int max) {
    int count = 0;
    for(int i=0; i<stored_auxc && i<max; i++) {
        buf[i] = stored_auxv[i];
        count++;
    }
    return count;
}
