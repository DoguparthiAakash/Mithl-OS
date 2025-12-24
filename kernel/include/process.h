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

// Forward declaration
struct fs_node;

// Process Control Block (PCB)
typedef struct process {
    int pid;                    // Process ID
    char name[32];              // Process Name
    char cmdline[128];          // Command Line Arguments
    process_state_t state;      // Current State
    
    uint32_t esp;               // Stack Pointer
    uint32_t ebp;               // Base Pointer
    uint32_t eip;               // Instruction Pointer (Entry)
    void *kernel_stack;         // Pointer to bottom of allocated kernel stack
    
    uint32_t page_directory;    // Physical Address of Page Directory (CR3)
    
    // UNIX-like additions
    int parent_pid;
    int exit_code;
    uint32_t heap_end;          // Program Break (brk) for heap expansion
    
    char cwd[256];              // Current Working Directory
    
    // File Descriptor Table
    // We need a helper struct to track offset per FD
    struct file_descriptor {
        struct fs_node *node;
        uint32_t offset;
        int flags;
    } *fd_table[256];
    
    struct process *next;       // Linked List
} process_t;

// Process Manager Functions
void process_init(void);
process_t *process_create(const char *name, void (*entry_point)(void));
process_t *process_create(const char *name, void (*entry_point)(void));
process_t *process_create_elf(const char *name, const char *filename, const char *args);
void process_schedule(void);
void process_yield(void);
void process_exit(void);
void process_init_main_thread(void);

// Helper struct for Listing (must match userspace)
typedef struct {
    int pid;
    char name[32];
    int state;
} process_info_t;

int process_get_list(process_info_t *buf, int max_count);

#endif
