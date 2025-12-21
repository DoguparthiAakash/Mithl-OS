#include "idt.h"
#include "console.h"
#include "gui.h"
#include "string.h"
#include "memory.h"
#include "process.h"

// Syscall Numbers (Linux Compatibility where possible)
#define SYS_EXIT      1
#define SYS_FORK      2
#define SYS_READ      3
#define SYS_WRITE     4
#define SYS_OPEN      5
#define SYS_CLOSE     6
#define SYS_EXECVE    11

// Custom Mithl-OS Syscalls
#define SYS_MITHL_GUI_CREATE  100
#define SYS_MITHL_GUI_BUTTON  101
#define SYS_MITHL_LOG         102

void syscall_handler(registers_t *regs) {
    console_write("[DEBUG] SYSCALL ENTERED. AX=");
    // print hex regs->eax
    if (regs->eax == 100) console_write("100(Create)\n");
    else if (regs->eax == 102) console_write("102(Log)\n");
    else console_write("Other\n");

    uint32_t syscall_nr = regs->eax;
    int ret = 0;

    switch (syscall_nr) {
        case SYS_WRITE:
            {
                // int fd = regs->ebx;
                char *buf = (char*)regs->ecx;
                uint32_t count = regs->edx;
                // For now, always write to console
                for (uint32_t i = 0; i < count; i++) {
                    char c = buf[i];
                    console_putc(c);
                }
                ret = count;
            }
            break;

        case SYS_EXECVE:
            {
                char *filename = (char*)regs->ebx;
                char **argv = (char**)regs->ecx;
                char **envp = (char**)regs->edx;
                
                console_write("[SYSCALL] Execve: ");
                console_write(filename);
                console_write("\n");
                
                extern int process_exec(const char *f, char *const a[], char *const e[]);
                ret = process_exec(filename, argv, envp);
            }
            break;
            
        case SYS_EXIT:
            {
                // int status = regs->ebx;
                console_write("[SYSCALL] Exit called.\n");
                
                extern void process_exit(void);
                process_exit();
                
                // Should not reach here
                while(1);
            }
            break;

        case SYS_MITHL_LOG:
            {
                char *msg = (char*)regs->ebx;
                console_write(msg);
                ret = 0;
            }
            break;

        case SYS_MITHL_GUI_CREATE:
            {
                // ebx=title, ecx=x, edx=y, esi=w, edi=h
                char *title = (char*)regs->ebx;
                console_write("[SYSCALL] Create Window: ");
                console_write(title);
                console_write("\n");
                
                // Create the window
                // Ensure we cast/convert types correctly if needed
                gui_window_t *win = gui_create_window(title, regs->ecx, regs->edx, regs->esi, regs->edi);
                
                // We shouldn't return a pointer to userspace directly if it's kernel memory, 
                // but for now let's just return 1 or the handle. 
                // In a real OS we'd return a handle ID. Use the pointer as ID for now.
                ret = (int)win; 
            }
            break;

        default:
            console_write("[SYSCALL] Unknown syscall: ");
            // print hex syscall_nr
            console_write("\n");
            ret = -1;
            break;
    }

    // Return value goes into EAX
    regs->eax = ret;
}
