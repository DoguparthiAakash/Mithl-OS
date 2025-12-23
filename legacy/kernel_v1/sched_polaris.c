/* kernel/sched/sched_polaris.c */
#include "sched_polaris.h"
#include "memory.h"
#include "string.h"
#include "console.h"

// Types mapping
#define kmalloc memory_alloc
#define kfree memory_free

static thread_t *running_thread = NULL;
static thread_t *thread_list_head = NULL;
static thread_t *thread_list_tail = NULL;
static int next_pid = 1;
static int next_tid = 1;

extern void switch_task(uint32_t *next_stack, uint32_t *current_stack);

void sched_init_polaris(void) {
    running_thread = NULL;
    thread_list_head = NULL;
    thread_list_tail = NULL;
    
    // Create Main Kernel Thread (Bootstrap)
    // We manually construct it representing the CURRENT execution flow.
    process_polaris_t *kproc = sched_create_process("Kernel");
    thread_t *kthread = (thread_t*)kmalloc(sizeof(thread_t));
    memset(kthread, 0, sizeof(thread_t));
    kthread->tid = 0;
    kthread->state = THREAD_RUNNING;
    kthread->process = kproc;
    kthread->priority = 0;
    
    list_append(kproc->threads, kthread);
    sched_add_thread(kthread);
    
    running_thread = kthread;
    
    console_write("[Polaris Sched] Initialized (bootstrap thread ready).\n");
}

process_polaris_t *sched_create_process(const char *name) {
    process_polaris_t *proc = (process_polaris_t*)kmalloc(sizeof(process_polaris_t));
    if (!proc) return NULL;
    
    memset(proc, 0, sizeof(process_polaris_t));
    proc->pid = next_pid++;
    strncpy(proc->name, name, 63);
    proc->threads = list_create();
    
    // TODO: Init Page Directory (CR3) if implementing virtual memory isolation
    // For now, share kernel directory (0) or similar
    
    return proc;
}

// Helper to init stack (Similar to process.c but for thread_t)
static void *prepare_stack_polaris(void *stack_base, void (*entry_point)(void)) {
    uint32_t *stack = (uint32_t *)stack_base;
    *--stack = (uint32_t)entry_point; // EIP
    *--stack = 0; // EBP
    *--stack = 0; // EBX
    *--stack = 0; // ESI
    *--stack = 0; // EDI
    *--stack = 0x202; // EFLAGS
    return stack;
}

thread_t *sched_create_thread(process_polaris_t *proc, void (*entry)(void)) {
    thread_t *t = (thread_t*)kmalloc(sizeof(thread_t));
    if (!t) return NULL;
    
    memset(t, 0, sizeof(thread_t));
    t->tid = next_tid++;
    t->state = THREAD_READY;
    t->process = proc;
    t->priority = 10;
    t->time_slice = 10;
    
    // Stack Allocation (4KB)
    void *stack = kmalloc(4096);
    t->kernel_stack = (uint32_t)stack + 4096;
    t->esp = (uint32_t)prepare_stack_polaris((void*)t->kernel_stack, entry);
    
    // Add to process list
    list_append(proc->threads, t);
    
    // Add to global scheduler list
    sched_add_thread(t);
    
    return t;
}

void sched_add_thread(thread_t *thread) {
    if (!thread) return;
    
    thread->next = NULL;
    
    if (!thread_list_head) {
        thread_list_head = thread;
        thread_list_tail = thread;
    } else {
        thread_list_tail->next = thread;
        thread_list_tail = thread;
    }
}

void sched_remove_thread(thread_t *thread) {
    // Simple linked list remove
    if (!thread_list_head || !thread) return;
    
    if (thread_list_head == thread) {
        thread_list_head = thread->next;
        if (!thread_list_head) thread_list_tail = NULL;
    } else {
        thread_t *prev = thread_list_head;
        while (prev->next && prev->next != thread) {
            prev = prev->next;
        }
        if (prev->next == thread) {
            prev->next = thread->next;
            if (prev->next == NULL) thread_list_tail = prev;
        }
    }
}

// Round Robin Scheduler (Soft Switch)
void sched_schedule_polaris(void) {
    if (!running_thread) return;

    // Save state of old thread
    // running_thread->esp is updated by switch_task logic (passed as pointer)
    
    // Pick next
    thread_t *next = running_thread->next;
    if (!next) next = thread_list_head; // Wrap around
    
    // Skip non-ready?
    thread_t *start = next;
    while (next && next != running_thread && next->state != THREAD_READY) {
        next = next->next;
        if (!next) next = thread_list_head;
        if (next == start) break; // No one ready
    }
    
    // If we found a candidate and it's not us (or even if it is us but we want to yield? No, optimization)
    if (next && next != running_thread && next->state == THREAD_READY) {
        thread_t *prev = running_thread;
        
        // Basic State Transition
        if (prev->state == THREAD_RUNNING) prev->state = THREAD_READY;
        
        running_thread = next;
        running_thread->state = THREAD_RUNNING;
        
        // Console logging for debug (optional, might spam)
        // console_log("Switching Task...\n");
        
        switch_task(&next->esp, &prev->esp);
    }
}

thread_t *sched_get_current_thread(void) {
    return running_thread;
}
