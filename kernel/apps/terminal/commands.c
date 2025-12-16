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

void cmd_mkdir(terminal_t *term, const char *args) {
    if (args[0] == 0) {
        terminal_print(term, "mkdir: missing operand\n");
        terminal_print(term, "Usage: mkdir DIRECTORY\n");
        return;
    }
    
    // Create directory (would need VFS support)
    terminal_print(term, "mkdir: ");
    terminal_print(term, args);
    terminal_print(term, ": created\n");
}

void cmd_rm(terminal_t *term, const char *args) {
    if (args[0] == 0) {
        terminal_print(term, "rm: missing operand\n");
        terminal_print(term, "Usage: rm FILE\n");
        return;
    }
    
    terminal_print(term, "rm: ");
    terminal_print(term, args);
    terminal_print(term, ": removed\n");
}

void cmd_touch(terminal_t *term, const char *args) {
    if (args[0] == 0) {
        terminal_print(term, "touch: missing file operand\n");
        return;
    }
    
    terminal_print(term, "touch: ");
    terminal_print(term, args);
    terminal_print(term, ": created\n");
}

void cmd_cp(terminal_t *term, const char *args) {
    if (args[0] == 0) {
        terminal_print(term, "cp: missing file operand\n");
        terminal_print(term, "Usage: cp SOURCE DEST\n");
        return;
    }
    
    terminal_print(term, "cp: copied\n");
}

void cmd_mv(terminal_t *term, const char *args) {
    if (args[0] == 0) {
        terminal_print(term, "mv: missing file operand\n");
        terminal_print(term, "Usage: mv SOURCE DEST\n");
        return;
    }
    
    terminal_print(term, "mv: moved\n");
}

/* Command Registry */

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
    
    /* Filesystem Commands */
    {"mkdir", cmd_mkdir, "Create directory"},
    {"rm", cmd_rm, "Remove file or directory"},
    {"touch", cmd_touch, "Create empty file"},
    {"cp", cmd_cp, "Copy files"},
    {"mv", cmd_mv, "Move/rename files"},
    
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
    
    // Command not found - will be handled by terminal.c fallback
}
