    if (boot_info.mod_count > 0) {
        console_write("[KERNEL] FIXED-SEARCH Searching for Init Module...\n");
        int init_idx = -1;
        
        for (uint32_t i=0; i < boot_info.mod_count; i++) {
             if (boot_info.modules[i].string && strstr((char*)boot_info.modules[i].string, "init")) {
                 init_idx = i;
                 break;
             }
        }
        
        if (init_idx == -1) {
             console_write("[KERNEL] Init module not found in list\n");
             serial_write("[KERNEL] Init module not found in list\n");
        } else {
            console_write("[KERNEL] Loading Init Module from found index\n");
            
            uint32_t start = boot_info.modules[init_idx].mod_start;
            
            extern uint32_t elf_load_from_memory(const char *name, void *addr, uint32_t size);
            uint32_t entry = elf_load_from_memory("Init", (void*)start, 0); 
            
            if (entry) {
                 console_write("[KERNEL] Executing Init...\n");
                 process_create("Init", (void (*)(void))entry);
            }
        }
    } else {
        console_write("[KERNEL] No Init Module found.\n");
        serial_write("[KERNEL] No Init Module found.\n");
    }
