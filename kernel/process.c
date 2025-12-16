#include "process.h"
#include "memory.h"
#include "string.h"
#include "console.h"

static process_t *process_list = NULL;
static process_t *current_process = NULL;
static int next_pid = 1;

extern void console_log(const char *msg);

void process_init(void) {
    process_list = NULL;
    current_process = NULL;
    console_log("[INFO] Process Manager Initialized.\n");
}

process_t *process_create(const char *name, void (*entry_point)(void)) {
    process_t *proc = (process_t *)memory_alloc(sizeof(process_t));
    if (!proc) return NULL;
    
    proc->pid = next_pid++;
    
    int len = 0; while(name[len]) len++;
    if (len > 31) len = 31;
    
    for(int i=0; i<len; i++) proc->name[i] = name[i];
    proc->name[len] = 0;
    
    proc->state = PROCESS_STATE_READY;
    proc->eip = (uint32_t)entry_point;
    
    // Stack and Context setup would happen here
    // For now, we assume cooperative or same-stack for simple apps
    
    proc->next = process_list;
    process_list = proc;
    
    console_log("[INFO] Process Created: ");
    console_log(proc->name);
    console_log("\n");
    
    return proc;
}

void process_schedule(void) {
    // Round Robin Stub
    if (!process_list) return;
    
    if (!current_process) {
        current_process = process_list;
    } else {
        current_process = current_process->next;
        if (!current_process) current_process = process_list;
    }
    
    if (current_process && current_process->state == PROCESS_STATE_READY) {
        current_process->state = PROCESS_STATE_RUNNING;
        // Context switch would happen here
    }
}

void process_exit(void) {
    if (current_process) {
        current_process->state = PROCESS_STATE_TERMINATED;
        process_schedule();
    }
}
