/* kernel/include/sched_polaris.h */
#ifndef SCHED_POLARIS_H
#define SCHED_POLARIS_H

#include "types.h"
#include "list.h"

// Thread States
typedef enum {
    THREAD_READY,
    THREAD_RUNNING,
    THREAD_BLOCKED,
    THREAD_TERMINATED
} thread_state_t;

// Process (Container)
typedef struct process_polaris {
    int pid;
    char name[64];
    uint32_t cr3; // Page Directory (Address Space)
    list_t *threads; // List of threads
    
    // Limits / Info
    uint32_t heap_start;
    uint32_t heap_end;
} process_polaris_t;

// Thread (Execution Unit)
typedef struct thread {
    int tid;
    thread_state_t state;
    
    // Context
    uint32_t esp;    // Stack Pointer
    uint32_t kernel_stack;
    
    // Parent Process
    process_polaris_t *process;
    
    // Scheduling
    int priority;
    int time_slice;
    
    struct thread *next; // For scheduler list
} thread_t;

// Scheduler API
void sched_init_polaris(void);
process_polaris_t *sched_create_process(const char *name);
thread_t *sched_create_thread(process_polaris_t *proc, void (*entry)(void));
void sched_add_thread(thread_t *thread);
void sched_remove_thread(thread_t *thread);

// The Core Switcher (called by timer IRQ)
void sched_schedule_polaris(void);

// Helper
thread_t *sched_get_current_thread(void);

#endif
