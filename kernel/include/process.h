#ifndef PROCESS_H
#define PROCESS_H

#include "types.h"

// Process States (like Unix/Windows)
typedef enum {
    PROCESS_STATE_READY,
    PROCESS_STATE_RUNNING,
    PROCESS_STATE_BLOCKED,
    PROCESS_STATE_TERMINATED
} process_state_t;

// Process Control Block (PCB)
typedef struct process {
    int pid;                    // Process ID
    char name[32];              // Process Name
    process_state_t state;      // Current State
    
    uint32_t esp;               // Stack Pointer
    uint32_t ebp;               // Base Pointer
    uint32_t eip;               // Instruction Pointer (Entry)
    
    uint32_t page_directory;    // Physical Address of Page Directory (CR3)
    
    struct process *next;       // Linked List
} process_t;

// Process Manager Functions
void process_init(void);
process_t *process_create(const char *name, void (*entry_point)(void));
process_t *process_create_elf(const char *name, const char *filename);
void process_schedule(void);
void process_exit(void);

#endif
