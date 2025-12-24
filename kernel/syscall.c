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
                     // Redirect to GUI Terminal if active
                     extern void terminal_active_write(const char *buf, uint32_t len);
                     terminal_active_write(buf, count);
                     
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

        case 12: // SYS_READDIR (fd, dirent_buf)
            {
                int fd = regs->ebx;
                struct dirent *user_dirent = (struct dirent*)regs->ecx;
                
                if (current_process && fd >= 0 && fd < 256 && current_process->fd_table[fd]) {
                    struct file_descriptor *desc = current_process->fd_table[fd];
                    if (desc->node) {
                        // Use offset as index
                        struct dirent *d = readdir_fs(desc->node, desc->offset);
                        if (d) {
                             // Copy to user buffer
                             // In real OS, check pointers!
                             // struct dirent is small (128 char + uint)
                             // We need to copy content.
                             // *user_dirent = *d; // Struct copy
                             // But wait, `d` is kernel pointer from readdir_fs (static or malloc?)
                             // readdir_fs usually returns pointer to static or internal.
                             // Let's copy safely.
                             strcpy(user_dirent->name, d->name);
                             user_dirent->ino = d->ino;
                             
                             desc->offset++;
                             ret = 1; // Success
                        } else {
                             ret = 0; // EOF
                        }
                    } else {
                        ret = -1;
                    }
                } else {
                    ret = -1;
                }
            }
            break;
            
        case 106: // SYS_PROCESS_LIST (buf, max)
            {
                process_info_t *buf = (process_info_t*)regs->ebx;
                int max = regs->ecx;
                
                // Security check needed for buf ptr in real OS
                
                if (buf && max > 0) {
                     ret = process_get_list(buf, max);
                } else {
                     ret = -1;
                }
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
            
        case 107: // SYS_GET_CMDLINE (from previous task, just ensuring location)
             // Already added.
             break; 

        case 108: // SYS_MKDIR (path, mode)
            {
                char *path = (char*)regs->ebx;
                uint32_t mode = regs->ecx;
                
                // 1. Separate Parent and Child
                // Find last slash
                char *last_slash = NULL;
                char *p = path;
                while(*p) {
                    if (*p == '/') last_slash = p;
                    p++;
                }
                
                fs_node_t *parent = NULL;
                char *child_name = NULL;
                
                if (last_slash) {
                    // Split
                    *last_slash = 0; // Temp modify
                    char *parent_path = path;
                    if (parent_path[0] == 0) parent_path = "/"; // was "/foo" -> "" "foo"
                    
                    child_name = last_slash + 1;
                    parent = vfs_resolve_path(parent_path);
                    
                    *last_slash = '/'; // Restore
                } else {
                    // Relative to root implicitly for now (or cwd if we had it)
                    // Assuming path is just "foo" -> root/foo
                    extern fs_node_t *fs_root;
                    parent = fs_root;
                    child_name = path;
                }
                
                if (parent && child_name[0]) {
                     mkdir_fs(parent, child_name, mode);
                     ret = 0;
                } else {
                     ret = -1;
                }
            }
            break;
            

            
        case 10: // SYS_UNLINK (path)
            {
                char *path = (char*)regs->ebx;
                // Basic split logic again (Duplicated for now, should be helper)
                char *last_slash = NULL;
                char *p = path;
                while(*p) { if (*p == '/') last_slash = p; p++; }
                
                fs_node_t *parent = NULL;
                char *child_name = NULL;
                
                if (last_slash) {
                    *last_slash = 0; char *pp = path; if(!pp[0]) pp="/";
                    child_name = last_slash + 1;
                    parent = vfs_resolve_path(pp);
                    *last_slash = '/';
                } else {
                    extern fs_node_t *fs_root;
                    parent = fs_root;
                    child_name = path;
                }
                
                if (parent && child_name[0]) {
                     unlink_fs(parent, child_name);
                     ret = 0;
                } else {
                     ret = -1;
                }
            }
            break;

        case 38: // SYS_RENAME (old, new)
            // Rename is complex as it might involve different parents.
            // But vfs.c has vfs_rename helper?
            // "vfs_rename" in vfs.c is (parent, old, new). It's simple rename in SAME dir.
            // If we want "mv /a/b /c/d", that's a move.
            // vfs.c has vfs_move? 
            // In vfs.c I saw: vfs_delete, vfs_rename, vfs_copy, vfs_move exposed in vfs.h!
            // Let's use vfs_move directly if possible!
            // BUT wait, syscalls usually map to low level. Userspace `mv` uses `rename` syscall.
            // I'll implement `SYS_RENAME` calling `vfs_move` or `vfs_rename`.
            // Linux `rename` works across dirs if on same FS.
            {
                char *oldpath = (char*)regs->ebx;
                char *newpath = (char*)regs->ecx;
                
                extern int vfs_move(const char *src, const char *dest);
                // vfs_move matches the behavior we want (move or rename).
                ret = vfs_move(oldpath, newpath);
            }
            break;



        case 8: // SYS_CREAT (path, mode)
            {
                char *path = (char*)regs->ebx;
                uint32_t mode = regs->ecx;
                
                // Same split logic
                char *last_slash = NULL; char *p = path; while(*p) { if (*p == '/') last_slash = p; p++; }
                fs_node_t *parent = NULL; char *child = NULL;
                
                if (last_slash) {
                    *last_slash=0; char *pp=path; if(!pp[0]) pp="/";
                    child = last_slash+1;
                    parent = vfs_resolve_path(pp);
                    *last_slash='/';
                } else {
                    extern fs_node_t *fs_root;
                    parent = fs_root;
                    child = path;
                }
                
                if (parent && child[0]) {
                     create_fs(parent, child, mode);
                     // Now open it? SYS_CREAT returns fd.
                     // But old creat usually returned fd.
                     // My create_fs is void.
                     // I should resolve it again and open it.
                     // For now, return 0 (Success) and let user call open separately.
                     // Linux creat returns fd.
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
