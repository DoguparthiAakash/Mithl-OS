#include "process.h"
#include "memory.h"
#include "string.h"
#include "console.h"
#include "mm/vmm.h"
#include "idt.h" // registers_t

static process_t *process_list = NULL;
process_t *current_process = NULL;
static int next_pid = 1;

extern void console_log(const char *msg);

void process_init(void) {
    process_list = NULL;
    current_process = NULL;
    console_log("[INFO] Process Manager Initialized.\n");
}

extern void switch_task(uint32_t *next_stack, uint32_t *current_stack);

// Helper to initialize stack for a new thread
static void *prepare_stack(void *stack_base, void (*entry_point)(void)) {
    uint32_t *stack = (uint32_t *)stack_base;
    
    // Simple stack layout matching switch_task:
    // [EIP] [EBP] [EBX] [ESI] [EDI] [EFLAGS]
    
    *--stack = (uint32_t)entry_point; // Return Address (EIP)
    *--stack = 0;                     // EBP
    *--stack = 0;                     // EBX
    *--stack = 0;                     // ESI
    *--stack = 0;                     // EDI
    *--stack = 0x202;                 // EFLAGS (Interrupts Enabled)
    
    return stack;
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
    
    // Allocate 4KB Stack
    void *stack_addr = memory_alloc(4096);
    if (!stack_addr) {
        // memory_free(proc);
        return NULL;
    }
    
    proc->kernel_stack = stack_addr;

    // Stack grows down, so base is at the end
    void *stack_top = (void*)((uint32_t)stack_addr + 4096);
    proc->esp = (uint32_t)prepare_stack(stack_top, entry_point);
    
    proc->esp = (uint32_t)prepare_stack(stack_top, entry_point);
    
    // Initialize UNIX fields
    proc->parent_pid = -1;
    proc->exit_code = 0;
    proc->heap_end = 0x10000000; // Start Heap at 256MB mark (Temporary safe zone)
    for(int i=0; i<256; i++) proc->fd_table[i] = NULL;
    
    // VMM: Clone Kernel Directory
    // Each process gets its own Address Space (initially copy of kernel)
    extern pd_entry_t* kernel_page_directory;
    proc->page_directory = (uint32_t)vmm_clone_directory(kernel_page_directory);
    
    proc->next = NULL;

    // Append to list (Round Robin)
    if (!process_list) {
        process_list = proc;
    } else {
        process_t *p = process_list;
        while (p->next) p = p->next;
        p->next = proc;
    }
    
    console_log("[INFO] Thread Created: ");
    console_log(proc->name);
    console_log("\n");
    
    return proc;
}

#include "elf.h"
extern uint32_t elf_load_file(const char *filename);

process_t *process_create_elf(const char *name, const char *filename, const char *args) {
    // 1. Load ELF
    uint32_t entry = elf_load_file(filename);
    if (!entry) {
        console_log("[PROCESS] Failed to load ELF: ");
        console_log(filename);
        console_log("\n");
        return NULL;
    }
    
    // 2. Create Process Wrapper
    // Cast entry point to function pointer
    process_t *proc = process_create(name, (void (*)(void))entry);
    
    // 3. Store Arguments
    if (proc && args) {
        int i=0;
        while(args[i] && i<127) {
            proc->cmdline[i] = args[i];
            i++;
        }
        proc->cmdline[i] = 0;
    }
    
    return proc;
}

void process_schedule(void) {
    if (!process_list) return;
    
    // If no current process, pick the first one (Bootstrap)
    if (!current_process) {
        current_process = process_list;
        // Should logically already be running if we are here? 
        // Or we are switching FROM init.
    }
    
    process_t *next = current_process->next;
    if (!next) next = process_list; // Loop Loop
    
    if (next == current_process) return; // Only 1 task
    
    process_t *prev = current_process;
    current_process = next;
    
    current_process->state = PROCESS_STATE_RUNNING;
    
    // UNIX VMM: Switch Address Space
    if (current_process->page_directory != prev->page_directory) {
        // Only switch if different (optimization)
        vmm_switch_pd((pd_entry_t*)current_process->page_directory);
    }
    
    // Context Switch
    switch_task(&next->esp, &prev->esp);
}

// Yield current time slice
void process_yield(void) {
    process_schedule();
}

// Exec: Replace current process image
int process_exec(const char *filename, char *const argv[], char *const envp[]) {
    // 1. Find file
    // For now, assume elf_load_file handles finding it via VFS
    // But elf_load_file returns 'entry point'. It loads into current address space?
    // Our elf_loader just maps pages.
    // If we are replacing, we should unmap old pages?
    // For Mithl-OS "Thread Wrapper" style, we can't easily replace.
    // BUT we must try.
    
    // Simplification: We spawn a NEW thread and kill the old one?
    // That's fork+exec. 
    // real exec replaces.
    
    // Let's implement 'spawn' semantics for now but call it exec.
    // Or clear current address space.
    
    // TODO: Clear address space.
    
    uint32_t entry = elf_load_file(filename);
    if (!entry) return -1;
    
    // Setup Stack with Argv
    // Argument passing in Linux:
    // Stack Top: argc
    //            argv pointers...
    //            0
    //            envp pointers...
    //            0
    //            Data (actual strings)
    
    // This is complex. For now, ignore args.
    
    // Jump to entry
    // We need to reset the stack and jump.
    // But we are in kernel mode C.
    // We can manually reset current_process->esp?
    
    // Hack: Just call it as function for now.
    ((void (*)(void))entry)();
    
    // If it returns, exit process
    process_exit();
    return 0;
}

void process_exit(void) {
    // Deschedule self
    if (!current_process) return;
    
    current_process->state = PROCESS_STATE_TERMINATED;
    
    console_log("[INFO] Process Exited\n");
    
    // Force switch effectively (never returns here)
    process_schedule();
    
    // If no processes left, halt
    while(1);
}

// Called by Kernel Main to convert the main thread into Task 0
void process_init_main_thread(void) {
    process_init();
    
    // Manually create PCB for current running context
    process_t *proc = (process_t *)memory_alloc(sizeof(process_t));
    proc->pid = 0;
    strcpy(proc->name, "Kernel/GUI");
    proc->state = PROCESS_STATE_RUNNING;
    proc->state = PROCESS_STATE_RUNNING;
    proc->parent_pid = -1;
    proc->exit_code = 0;
    // VMM: Main Thread uses Kernel PD
    extern pd_entry_t* kernel_page_directory;
    proc->page_directory = (uint32_t)kernel_page_directory;
    
    proc->next = NULL;
    
    process_list = proc;
    current_process = proc;
}

int process_get_list(process_info_t *buf, int max_count) {
    int count = 0;
    process_t *curr = process_list;
    while(curr && count < max_count) {
        buf[count].pid = curr->pid;
        strcpy(buf[count].name, curr->name);
        buf[count].state = curr->state;
        
        count++;
        curr = curr->next;
    }
    return count;
}

// UNIX Fork implementation
int process_fork(registers_t *regs) {
    // 1. Allocate Child PCB
    process_t *child = (process_t *)memory_alloc(sizeof(process_t));
    if (!child) return -1;
    
    // 2. Clone Identity
    child->pid = next_pid++; // Global
    child->parent_pid = current_process->pid;
    // Name
    int len=0; while(current_process->name[len]) len++;
    for(int i=0; i<32; i++) child->name[i] = current_process->name[i];
    
    // 3. Clone VMM
    // Note: This relies on vmm_clone_directory linking/copying correctly.
    // Ideally this does Deep Copy of user pages.
    extern pd_entry_t* kernel_page_directory;
    child->page_directory = (uint32_t)vmm_clone_directory((pd_entry_t*)current_process->page_directory);
    // If fail?
    
    // 4. Clone Stack
    // Alloc new kernel stack
    child->kernel_stack = memory_alloc(4096);
    if (!child->kernel_stack) return -1;
    
    // Copy the ENTIRE kernel stack contents from parent
    // current_process->kernel_stack should be valid.
    // If it's NULL (main thread), we are in trouble. Main thread shouldn't fork usually.
    // Assuming process created via process_create.
    if (!current_process->kernel_stack) {
        // Fallback or Error?
        // Main Init thread (PID 0) has no kernel_stack tracked.
        // Can we fork PID 0? The terminal runs as a process created by create_process?
        // If "Terminal" calls fork (cc.elf calls fork), checking current_process.
        // Yes, Terminal was created with process_create.
        return -1;
    }
    
    // Valid kernel_stack
    memcpy(child->kernel_stack, current_process->kernel_stack, 4096);
    
    // Adjust ESP
    // Calculate offset of ESP relative to stack base
    // Use the *current* ESP? "regs" is passed, but ESP in PCB might be old (from switch_task).
    // Wait. "regs" comes from syscall_handler. The syscall handler is running on the stack.
    // The ESP at the time of fork call is 'regs' + sizeof(registers_t)?
    // No, we want the stack state *saved*?
    // We want the child to resume exactly where parent is (inside syscall handler).
    // But on the new stack.
    
    // To identify "current stack pointer", we can take address of local var?
    uint32_t current_esp;
    asm volatile("mov %%esp, %0" : "=r"(current_esp));
    
    uint32_t offset = current_esp - (uint32_t)current_process->kernel_stack;
    child->esp = (uint32_t)child->kernel_stack + offset;
    
    // 5. Fix Return Value (EAX) in Child's Stack
    // 'regs' points to the trap frame on the PARENT stack.
    // We need to find where this trap frame is on the CHILD stack.
    uint32_t regs_offset = (uint32_t)regs - (uint32_t)current_process->kernel_stack;
    registers_t *child_regs = (registers_t *)((uint32_t)child->kernel_stack + regs_offset);
    
    child_regs->eax = 0; // Child sees 0
    
    // 6. Clone File Descriptors
    for(int i=0; i<256; i++) {
        if (current_process->fd_table[i]) {
            struct file_descriptor *new_desc = (struct file_descriptor*)memory_alloc(sizeof(struct file_descriptor));
            memcpy(new_desc, current_process->fd_table[i], sizeof(struct file_descriptor));
            
            // Note: We copy the struct, which points to the SAME fs_node.
            // This is correct (shared offset? No, offset is in struct file_descriptor).
            // fork() duplicates FD table, so child has INDEPENDENT offset?
            // "The child inherits copies of the parent's set of open file descriptors."
            // "The two file descriptors share a common open file description."
            // "Current file offset is shared."
            // WAIT. If they share offset, they must share the SAME struct file_descriptor?
            // Or a level of indirection?
            // In POSIX: "file descriptor" -> "open file description" (offset, status) -> "v-node".
            // Implementation: Parent and Child share the "open file description".
            // So we should just COPY THE POINTER?
            // "ref_count" needed to know when to free?
            // Current implementation has no ref_count in `struct file_descriptor`.
            // If we copy pointer, both modify same offset. This is Unix behavior.
            // But if one closes, it frees it? Then other crashes.
            // We NEED ref counting for shared FDs.
            
            // Since we don't have ref counting yet, DEEP COPY causes separate offsets.
            // This is safer for stability, but divergent from POSIX (offset not shared).
            // For V1 compilation/running `cc` or `make`, separate is likely OK.
            // I will do DEEP COPY for now to avoid use-after-free crashes.
            
            child->fd_table[i] = new_desc;
        } else {
            child->fd_table[i] = NULL;
        }
    }
    
    child->state = PROCESS_STATE_READY;
    child->next = NULL;
    
    // Append
    if (!process_list) process_list = child;
    else {
        process_t *p = process_list;
        while(p->next) p=p->next;
        p->next = child;
    }
    
    return child->pid; // Parent sees PID
}

int process_waitpid(int pid, int *status, int options) {
    (void)options; // Unused for now
    
    while(1) {
        // 1. Scan for child
        process_t *p = process_list;
        int found = 0;
        process_t *child = NULL;
        
        while(p) {
            if (p->pid == pid) {
                child = p;
                found = 1;
                break;
            }
            p = p->next;
        }
        
        if (!found) return -1; // ECHILD
        
        // 2. Check State
        if (child->state == PROCESS_STATE_TERMINATED) {
            if (status) *status = child->exit_code;
            
            // Cleanup Child (Zombie reaping)
            // Remove from list
            if (process_list == child) {
                process_list = child->next;
            } else {
                process_t *prev = process_list;
                while(prev->next != child) prev = prev->next;
                prev->next = child->next;
            }
            
            // Free Resources
            if (child->kernel_stack) memory_free(child->kernel_stack);
            // vmm_free_directory handled in exit (implicit)?
            // Wait, process_exit frees PD immediately unless shared.
            // So resources are mostly gone.
            memory_free(child);
            
            return pid;
        }
        
        // 3. Busy Wait Yield
        process_yield();
    }
}
