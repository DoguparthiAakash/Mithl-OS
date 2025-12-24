#include "idt.h"
#include "console.h"
#include "gui.h"
#include "string.h"
#include "memory.h"
#include "process.h"
#include "process.h"
#include "vfs.h"

extern process_t *current_process;

// Syscall Numbers (Linux Compatibility where possible)
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
                // char *buf = (char*)regs->ecx;
                // uint32_t count = regs->edx;
                
                int fd = regs->ebx;
                char *buf = (char*)regs->ecx;
                uint32_t count = regs->edx;
                
                // If STDOUT/STDERR (1 or 2), verify logic?
                // For now, hardcode FD 1/2 to console if not in table?
                // Or checking table.
                
                if (fd == 1 || fd == 2) {
                     for (uint32_t i = 0; i < count; i++) {
                        console_putc(buf[i]);
                     }
                     ret = count;
                } else if (current_process && fd >= 0 && fd < 256 && current_process->fd_table[fd]) {
                     // Read from FILE? SYS_WRITE to FILE?
                     // Ah, this is WRITE. 
                     struct file_descriptor *desc = current_process->fd_table[fd];
                     // TODO: Implement write_fs logic tracking offset
                     // For now, support stdout only.
                     ret = 0; 
                } 
            }
            break;

        case SYS_READ:
            {
                int fd = regs->ebx;
                char *buf = (char*)regs->ecx;
                uint32_t count = regs->edx;
                
                if (current_process && fd >= 0 && fd < 256 && current_process->fd_table[fd]) {
                    struct file_descriptor *desc = current_process->fd_table[fd];
                    if (desc->node) {
                        uint32_t read_bytes = read_fs(desc->node, desc->offset, count, (uint8_t*)buf);
                        desc->offset += read_bytes;
                        ret = read_bytes;
                    } else {
                        ret = -1;
                    }
                } else {
                    ret = -1; // Bad FD
                }
            }
            break;

        case SYS_OPEN:
            {
                char *path = (char*)regs->ebx;
                // int flags = regs->ecx;
                
                fs_node_t *node = vfs_resolve_path(path);
                if (!node) {
                    ret = -1; // ENOENT
                } else {
                    // Find free FD
                    int fd = -1;
                    for(int i=3; i<256; i++) { // Reserved 0,1,2
                         if (current_process->fd_table[i] == NULL) {
                             fd = i;
                             break;
                         }
                    }
                    
                    if (fd != -1) {
                        // Allocate descriptor
                        struct file_descriptor *desc = (struct file_descriptor*)memory_alloc(sizeof(struct file_descriptor));
                        desc->node = node;
                        desc->offset = 0;
                        desc->flags = 0;
                        current_process->fd_table[fd] = desc;
                        
                        // Open VFS hook
                        open_fs(node, 1, 0); 
                        
                        ret = fd;
                    } else {
                        ret = -1; // EMFILE
                    }
                }
            }
            break;
            
        case SYS_CLOSE:
            {
                int fd = regs->ebx;
                if (current_process && fd >= 3 && fd < 256 && current_process->fd_table[fd]) {
                     struct file_descriptor *desc = current_process->fd_table[fd];
                     close_fs(desc->node);
                     memory_free(desc);
                     current_process->fd_table[fd] = NULL;
                     ret = 0;
                } else {
                     ret = -1;
                }
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
                
                // We shouldn't return a pointer to userspace directly if it's right kernel memory, 
                // but for now let's just return 1 or the handle. 
                // In a real OS we'd return a handle ID. Use the pointer as ID for now.
                ret = (int)win; 
            }
            break;

        case 103: // SYS_GET_EVENT
            {
                gui_event_t *user_buf = (gui_event_t*)regs->ebx;
                if (!user_buf) { ret = -1; break; }

                // Find window owned by current process (Simple Linear Search for now)
                // In future, pass window handle ID
                gui_window_t *found = NULL;
                
                // Assuming root->children contains windows (Layout Manager dependent)
                if (gui_mgr.root && gui_mgr.root->children) {
                    list_node_t *node = gui_mgr.root->children->head;
                    while (node) {
                        gui_element_t *el = (gui_element_t*)node->data;
                        if (el->type == GUI_ELEMENT_WINDOW) {
                             gui_window_t *w = (gui_window_t*)el;
                             if (w->owner_pid == current_process->pid) {
                                 found = w;
                                 break;
                             }
                        }
                        node = node->next;
                    }
                }
                
                if (found && found->incoming_events && found->incoming_events->head) {
                    gui_event_t *ev = (gui_event_t*)list_pop_front(found->incoming_events);
                    if (ev) {
                        // Copy to user buffer
                        // Note: Be careful with direct pointer deref if user address is bad (kernel checks needed normally)
                        *user_buf = *ev; 
                        memory_free(ev);
                        ret = 1; // Event retrieved
                    } else {
                        ret = 0;
                    }
                } else {
                    ret = 0; // No event
                }
            }
            break;

        case 104: // SYS_DRAW_RECT (x, y, w, h, color)
            {
                 // We need 5 args. regs->ebx, ecx, edx, esi, edi
                 // ebx=x, ecx=y, edx=w, esi=h, edi=color
                 int x = regs->ebx;
                 int y = regs->ecx;
                 int w = regs->edx;
                 int h = regs->esi;
                 uint32_t color = regs->edi;

                 // Find Window (Refactor this later)
                 gui_window_t *found = NULL;
                 if (gui_mgr.root && gui_mgr.root->children) {
                    list_node_t *node = gui_mgr.root->children->head;
                    while (node) {
                        gui_element_t *el = (gui_element_t*)node->data;
                        if (el->type == GUI_ELEMENT_WINDOW) {
                             gui_window_t *win = (gui_window_t*)el;
                             if (win->owner_pid == current_process->pid) {
                                 found = win;
                                 break;
                             }
                        }
                        node = node->next;
                    }
                 }
                 
                 if (found) {
                     // Clip and Offset
                     int title_h = 30;
                     int win_x = found->base.bounds.x;
                     int win_y = found->base.bounds.y + title_h;
                     
                     // Absolute coords
                     int abs_x = win_x + x;
                     int abs_y = win_y + y;
                     
                     // Basic Clipping against window bounds (not perfect, but safe)
                     if (abs_x < win_x) abs_x = win_x;
                     if (abs_y < win_y) abs_y = win_y;
                     // width/height clipping todo
                     
                     rect_t r = {abs_x, abs_y, w, h};
                     draw_rect_filled(r, color);
                     gui_invalidate_rect(r); // Mark dirty
                     ret = 0;
                 } else {
                     ret = -1;
                 }
            }
            break;
            
        case 105: // SYS_DRAW_TEXT (msg, x, y, color)
            {
                 char *msg = (char*)regs->ebx;
                 int x = regs->ecx;
                 int y = regs->edx;
                 uint32_t color = regs->esi;
                 
                 gui_window_t *found = NULL;
                 if (gui_mgr.root && gui_mgr.root->children) {
                    list_node_t *node = gui_mgr.root->children->head;
                    while (node) {
                        gui_element_t *el = (gui_element_t*)node->data;
                        if (el->type == GUI_ELEMENT_WINDOW) {
                             gui_window_t *win = (gui_window_t*)el;
                             if (win->owner_pid == current_process->pid) {
                                 found = win;
                                 break;
                             }
                        }
                        node = node->next;
                    }
                 }
                 
                 if (found) {
                     int title_h = 30;
                     int abs_x = found->base.bounds.x + x;
                     int abs_y = found->base.bounds.y + title_h + y;
                     
                     draw_text(msg, abs_x, abs_y, color, 12); // Default size 12
                     // Invalidate text area (guesstimate)
                     gui_invalidate_rect((rect_t){abs_x, abs_y, strlen(msg)*10, 16});
                     ret = 0;
                 } else {
                     ret = -1;
                 }
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
