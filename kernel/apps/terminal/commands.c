#include "commands.h"
#include "terminal.h"
#include "string.h"
#include "memory.h"
#include "vfs.h"

/* External functions */
extern void shutdown_system(void);
extern void restart_system(void);
extern void terminal_print(terminal_t *term, const char *str);
extern fs_node_t *fs_root;

/* Current working directory (global for now) */
static char current_dir[256] = "/home/aakash";

const char* get_current_dir(void) {
    return current_dir;
}

/* Helper: String starts with */
static int starts_with(const char *str, const char *prefix) {
    while (*prefix) {
        if (*str++ != *prefix++) return 0;
    }
    return 1;
}

/* Helper: Parse command and arguments */
static void parse_cmdline(const char *cmdline, char *cmd, char *args) {
    int i = 0;
    
    // Skip leading spaces
    while (*cmdline == ' ') cmdline++;
    
    // Extract command
    while (*cmdline && *cmdline != ' ') {
        cmd[i++] = *cmdline++;
    }
    cmd[i] = 0;
    
    // Skip spaces before args
    while (*cmdline == ' ') cmdline++;
    
    // Copy remaining as args
    strcpy(args, cmdline);
}

/* Helper: Resolve argument to absolute path */
static void resolve_absolute_path(const char *arg, char *out_path) {
    if (arg[0] == '/') {
        // Already absolute
        strcpy(out_path, arg);
    } else {
        // Relative: join current_dir + / + arg
        strcpy(out_path, current_dir);
        if (strcmp(current_dir, "/") != 0) { // Don't double slash if root
            int len = strlen(out_path);
            out_path[len] = '/';
            out_path[len+1] = 0;
        }
        
        // Handle .. (simplified)
        if (strcmp(arg, "..") == 0) {
            // strip last component
            int len = strlen(out_path);
            if (len > 1) { // not root
                // remove trailing slash if exists
                if (out_path[len-1] == '/') out_path[len-1] = 0;
                
                // find last slash
                char *last = out_path;
                char *p = out_path;
                while (*p) {
                    if (*p == '/') last = p;
                    p++;
                }
                if (last != out_path) *last = 0; // Cut off
                else *(last+1) = 0; // Root
            }
        } else if (strcmp(arg, ".") == 0) {
             // do nothing
        } else {
            // Append arg
            int len = strlen(out_path);
            // check if trailing slash needed? already added above
            
            // Safe concat
            char *d = out_path + len;
            const char *s = arg;
            while (*s) *d++ = *s++;
            *d = 0;
        }
    }
}

/* File Commands */

void cmd_ls(terminal_t *term, const char *args) {
    char path[256];
    if (args[0] == 0) {
        strcpy(path, current_dir);
    } else {
        resolve_absolute_path(args, path);
    }
    
    fs_node_t *node = vfs_resolve_path(path);
    if (!node) {
        terminal_print(term, "ls: cannot access '");
        terminal_print(term, args[0] ? args : path);
        terminal_print(term, "': No such file or directory\n");
        return;
    }
    
    if ((node->flags & 0x7) == FS_DIRECTORY) {
        int index = 0;
        while (1) {
            struct dirent *d = readdir_fs(node, index);
            if (!d) break;
            
            terminal_print(term, d->name);
            terminal_print(term, "  ");
            
            index++;
        }
        if (index > 0) terminal_print(term, "\n");
    } else {
        terminal_print(term, args);
        terminal_print(term, "\n");
    }
}

void cmd_cd(terminal_t *term, const char *args) {
    if (args[0] == 0) {
        // Go home
        strcpy(current_dir, "/home/aakash");
        return;
    }
    
    char path[256];
    resolve_absolute_path(args, path);
    
    fs_node_t *node = vfs_resolve_path(path);
    if (node && (node->flags & 0x7) == FS_DIRECTORY) {
        strcpy(current_dir, path);
    } else {
        terminal_print(term, "cd: ");
        terminal_print(term, args);
        terminal_print(term, ": No such file or directory\n");
    }
}

void cmd_pwd(terminal_t *term, const char *args) {
    (void)args;
    terminal_print(term, current_dir);
    terminal_print(term, "\n");
}

void cmd_cat(terminal_t *term, const char *args) {
    if (args[0] == 0) {
        terminal_print(term, "cat: missing file operand\n");
        return;
    }
    
    char path[256];
    resolve_absolute_path(args, path);
    
    fs_node_t *node = vfs_resolve_path(path);
    if (!node) {
        terminal_print(term, "cat: ");
        terminal_print(term, args);
        terminal_print(term, ": No such file or directory\n");
        return;
    }
    
    if ((node->flags & 0x7) == FS_DIRECTORY) {
        terminal_print(term, "cat: ");
        terminal_print(term, args);
        terminal_print(term, ": Is a directory\n");
        return;
    }
    
    // Read file
    uint32_t size = node->length;
    char *buf = (char*)memory_alloc(size + 1);
    if (!buf) {
        terminal_print(term, "cat: out of memory\n");
        return;
    }
    
    uint32_t read = read_fs(node, 0, size, (uint8_t*)buf);
    buf[read] = 0;
    
    terminal_print(term, buf);
    terminal_print(term, "\n");
    
    // memory_free(buf); // TODO: Implement free
}

/* System Commands */

void cmd_shutdown(terminal_t *term, const char *args) {
    (void)args;
    terminal_print(term, "Shutting down system...\n");
    terminal_print(term, "Goodbye!\n");
    
    // Small delay to show message
    for (volatile int i = 0; i < 10000000; i++);
    
    shutdown_system();
}

void cmd_restart(terminal_t *term, const char *args) {
    (void)args;
    terminal_print(term, "Restarting system...\n");
    
    // Small delay
    for (volatile int i = 0; i < 10000000; i++);
    
    restart_system();
}

void cmd_halt(terminal_t *term, const char *args) {
    (void)args;
    terminal_print(term, "System halted. You can power off now.\n");
    
    __asm__ volatile("cli; hlt");
}

/* Text Utilities */

void cmd_echo(terminal_t *term, const char *args) {
    terminal_print(term, args);
    terminal_print(term, "\n");
}

/* System Information */

void cmd_uname(terminal_t *term, const char *args) {
    if (args[0] == 0) {
        terminal_print(term, "Mithl\n");
    } else if (strcmp(args, "-a") == 0) {
        terminal_print(term, "Mithl OS 1.0 i686 x86_32\n");
    } else if (strcmp(args, "-s") == 0) {
        terminal_print(term, "Mithl\n");
    } else if (strcmp(args, "-r") == 0) {
        terminal_print(term, "1.0\n");
    } else if (strcmp(args, "-m") == 0) {
        terminal_print(term, "i686\n");
    } else {
        terminal_print(term, "uname: invalid option\n");
        terminal_print(term, "Try: uname [-a|-s|-r|-m]\n");
    }
}

void cmd_free(terminal_t *term, const char *args) {
    (void)args;
    
    // Get memory info (these would be real functions)
    extern uint32_t pmm_get_total_memory(void);
    extern uint32_t pmm_get_free_memory(void);
    
    uint32_t total = pmm_get_total_memory();
    uint32_t free = pmm_get_free_memory();
    uint32_t used = total - free;
    
    terminal_print(term, "              total        used        free\n");
    terminal_print(term, "Mem:      ");
    
    // Simple number printing (no sprintf available)
    char buf[32];
    int i = 0;
    uint32_t val = total / 1024; // KB
    if (val == 0) buf[i++] = '0';
    else {
        char temp[32];
        int j = 0;
        while (val > 0) {
            temp[j++] = '0' + (val % 10);
            val /= 10;
        }
        while (j > 0) buf[i++] = temp[--j];
    }
    buf[i] = 0;
    terminal_print(term, buf);
    terminal_print(term, " KB    ");
    
    // Used
    i = 0;
    val = used / 1024;
    if (val == 0) buf[i++] = '0';
    else {
        char temp[32];
        int j = 0;
        while (val > 0) {
            temp[j++] = '0' + (val % 10);
            val /= 10;
        }
        while (j > 0) buf[i++] = temp[--j];
    }
    buf[i] = 0;
    terminal_print(term, buf);
    terminal_print(term, " KB    ");
    
    // Free
    i = 0;
    val = free / 1024;
    if (val == 0) buf[i++] = '0';
    else {
        char temp[32];
        int j = 0;
        while (val > 0) {
            temp[j++] = '0' + (val % 10);
            val /= 10;
        }
        while (j > 0) buf[i++] = temp[--j];
    }
    buf[i] = 0;
    terminal_print(term, buf);
    terminal_print(term, " KB\n");
}

void cmd_uptime(terminal_t *term, const char *args) {
    (void)args;
    terminal_print(term, "up 0 days, 0:05\n");
}

/* Filesystem Commands */

/* Filesystem Commands */

void cmd_mkdir(terminal_t *term, const char *args) {
    if (args[0] == 0) {
        terminal_print(term, "mkdir: missing operand\n");
        terminal_print(term, "Usage: mkdir DIRECTORY\n");
        return;
    }
    
    // Resolve parent path
    char path[256];
    resolve_absolute_path(args, path);
    
    // Split into parent and target name
    // (Assuming simple case: parent exists)
    // Find last slash
    char *last_slash = path;
    char *p = path;
    while (*p) { 
        if (*p == '/') last_slash = p; 
        p++; 
    }
    
    char parent_path[256];
    char target_name[128];
    
    if (last_slash == path) {
        // Root parent
        strcpy(parent_path, "/");
        strcpy(target_name, last_slash + 1);
    } else {
        int len = last_slash - path;
        strncpy(parent_path, path, len);
        parent_path[len] = 0;
        strcpy(target_name, last_slash + 1);
    }
    
    fs_node_t *parent = vfs_resolve_path(parent_path);
    if (!parent || (parent->flags & 0x7) != FS_DIRECTORY) {
        terminal_print(term, "mkdir: invalid parent directory\n");
        return;
    }
    
    mkdir_fs(parent, target_name, 0755);
    terminal_print(term, "Directory created.\n");
}

void cmd_rm(terminal_t *term, const char *args) {
    if (args[0] == 0) {
        terminal_print(term, "rm: missing operand\n");
        return;
    }
    
    char path[256];
    resolve_absolute_path(args, path);
    
    // Parent/Child splits logic
    char *last_slash = path;
    char *p = path;
    while (*p) { if (*p == '/') last_slash = p; p++; }
    
    char parent_path[256];
    char target_name[128];
    
    if (last_slash == path) {
        strcpy(parent_path, "/");
        strcpy(target_name, last_slash + 1);
    } else {
        int len = last_slash - path;
        strncpy(parent_path, path, len);
        parent_path[len] = 0;
        strcpy(target_name, last_slash + 1);
    }
    
    fs_node_t *parent = vfs_resolve_path(parent_path);
    if (parent) {
        unlink_fs(parent, target_name);
        terminal_print(term, "Deleted.\n");
    } else {
        terminal_print(term, "rm: parent not found\n");
    }
}

void cmd_cp(terminal_t *term, const char *args) {
    // Basic CP: cp src dest
    // Needs parsing of two args.
    // For now, let's just claim support is basic.
    terminal_print(term, "cp: Basic implementation (TODO: Arg parsing)\n");
}

void cmd_edit(terminal_t *term, const char *args) {
    // Usage: edit filename "content"
    // Rudimentary "echo content > file"
    
    // Split filename and content
    // Find space
    char filename[128];
    const char *content_start = 0;
    
    int i = 0;
    const char *p = args;
    while (*p && *p != ' ') {
        filename[i++] = *p++;
    }
    filename[i] = 0;
    
    if (*p == ' ') content_start = p + 1;
    
    if (filename[0] == 0 || !content_start) {
        terminal_print(term, "Usage: edit <filename> <content>\n");
        return;
    }
    
    char path[256];
    resolve_absolute_path(filename, path);
    
    fs_node_t *node = vfs_resolve_path(path);
    if (!node) {
        // Create if not exists?
        // Reuse touch logic... simplified:
        // Or just fail
        terminal_print(term, "File not found. Use 'touch' first.\n");
        return;
    }
    
    if ((node->flags & FS_FILE) != FS_FILE) {
        terminal_print(term, "Not a file.\n");
        return;
    }
    
    // Remove quotes if present?
    const char *data = content_start;
    if (data[0] == '"') {
        data++;
        // Remove trailing quote if exists?
        // We'll just write what's there for now.
    }
    
    write_fs(node, 0, strlen(data), (uint8_t*)data);
    terminal_print(term, "File updated.\n");
}

    // Re-injecting missing functions
void cmd_touch(terminal_t *term, const char *args) {
    if (args[0] == 0) {
        terminal_print(term, "touch: missing file operand\n");
        return;
    }
    
    char path[256];
    resolve_absolute_path(args, path);
    // Simple logic
    char *last_slash = path;
    char *p = path;
    while (*p) { if (*p == '/') last_slash = p; p++; }
    
    char parent_path[256];
    char target_name[128];
    
    if (last_slash == path) {
        strcpy(parent_path, "/");
        strcpy(target_name, last_slash + 1);
    } else {
        int len = last_slash - path;
        strncpy(parent_path, path, len);
        parent_path[len] = 0;
        strcpy(target_name, last_slash + 1);
    }

    fs_node_t *parent = vfs_resolve_path(parent_path);
    if (!parent) {
        terminal_print(term, "touch: invalid parent directory\n");
        return;
    }

    create_fs(parent, target_name, 0644);
    terminal_print(term, "File created.\n");
}

void cmd_mv(terminal_t *term, const char *args) {
    (void)args;
    terminal_print(term, "mv: not implemented\n");
}

static const command_t commands[] = {
    /* System Commands */
    {"shutdown", cmd_shutdown, "Shutdown the system"},
    {"restart", cmd_restart, "Restart the system"},
    {"reboot", cmd_restart, "Reboot the system"},
    {"halt", cmd_halt, "Halt the system"},
    {"poweroff", cmd_shutdown, "Power off the system"},
    
    /* System Information */
    {"uname", cmd_uname, "Print system information"},
    {"free", cmd_free, "Display memory usage"},
    {"uptime", cmd_uptime, "Show system uptime"},
    
    /* Text Utilities */
    {"echo", cmd_echo, "Display a line of text"},
    {"edit", cmd_edit, "Write text to file"},
    
    /* Filesystem Commands */
    {"mkdir", cmd_mkdir, "Create directory"},
    {"rm", cmd_rm, "Remove file or directory"},
    {"touch", cmd_touch, "Create empty file"},
    {"cp", cmd_cp, "Copy files"},
    {"mv", cmd_mv, "Move/rename files"},
    
    /* File Commands */
    {"ls", cmd_ls, "List directory contents"},
    {"cd", cmd_cd, "Change directory"},
    {"pwd", cmd_pwd, "Print working directory"},
    {"cat", cmd_cat, "Concatenate and display file content"},

    {NULL, NULL, NULL} // Sentinel
};

const command_t* get_commands(void) {
    return commands;
}

void execute_command(terminal_t *term, const char *cmdline) {
    char cmd[64], args[192];
    
    parse_cmdline(cmdline, cmd, args);
    
    if (cmd[0] == 0) return; // Empty command
    
    // Search for command
    for (int i = 0; commands[i].name; i++) {
        if (strcmp(cmd, commands[i].name) == 0) {
            commands[i].handler(term, args);
            return;
        }
    }
    // Command not found
}
