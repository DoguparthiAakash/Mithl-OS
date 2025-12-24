#include "../../libc/stdlib.h"

// Auxiliary Vector Types
#define AT_NULL   0
#define AT_IGNORE 1
#define AT_PHDR   3
#define AT_ENTRY  9
#define AT_BASE   7

// Stack Layout at Entry:
// [argc]
// [argv0] ... [argvN]
// [0]
// [envp0] ... [envpN]
// [0]
// [auxv0] [auxv1] ... 
// [0, 0]

// Helper to print hex
void print_hex(uint32_t val) {
    char buf[16];
    int i = 0;
    if (val == 0) {
        print("0x0");
        return;
    }
    
    // convert
    char chars[] = "0123456789ABCDEF";
    while(val) {
        buf[i++] = chars[val % 16];
        val /= 16;
    }
    print("0x");
    while(i > 0) {
        char c[2]; c[0] = buf[--i]; c[1] = 0;
        print(c);
    }
}

void start_linker(void *stack_top) {
    print("\n\n=== MITHL-OS MOCK DYNAMIC LINKER ===\n");
    print("Successfully loaded by Kernel via PT_INTERP!\n");
    
    // Inspect Stack
    uint32_t *sp = (uint32_t*)stack_top;
    int argc = *sp;
    print("ARGC: "); print_hex(argc); print("\n");
    
    // Skip Argc
    sp++;
    // Skip Argv (argc + 1 null)
    sp += (argc + 1);
    
    // Skip Envp (loop till null)
    while(*sp != 0) sp++;
    sp++; // Skip NULL
    
    // Now at AuxV
    print("Parsing Aux Vector...\n");
    
    struct { uint32_t type; uint32_t val; } *aux = (void*)sp;
    
    uint32_t entry = 0;
    
    while(aux->type != AT_NULL) {
        if (aux->type == AT_ENTRY) {
             print("Found AT_ENTRY (Main App): ");
             print_hex(aux->val);
             print("\n");
             entry = aux->val;
        }
        else if (aux->type == AT_BASE) {
             print("Found AT_BASE (Interpreter Base): ");
             print_hex(aux->val);
             print("\n");
        }
        else if (aux->type == AT_PHDR) {
             print("Found AT_PHDR: ");
             print_hex(aux->val);
             print("\n");
        }
        aux++;
    }
    
    print("=== MOCK LINKER INITIALIZATION COMPLETE ===\n");
    
    if (entry) {
        print("Transferring control to Main Application...\n");
        // Jump to entry
        // Note: In real linker, we would clean registers and jump.
        // For C, we can just call it (stack is slightly dirty but functional for V1)
        ((void (*)(void))entry)();
    } else {
        print("Error: No AT_ENTRY found.\n");
        exit(1);
    }
    
    exit(0);
}

// Entry Point
void _start() {
    // Get Stack Pointer
    void *sp;
    asm volatile("mov %%esp, %0" : "=r"(sp));
    start_linker(sp);
}
