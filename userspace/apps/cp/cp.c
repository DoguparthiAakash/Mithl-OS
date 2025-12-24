#include "../../libc/stdlib.h"

int strlen(const char *s) {
    int l=0; while(s[l]) l++; return l;
}

void print_err(const char *s) {
    print(s);
}

int main() {
    char args[128];
    if (get_cmdline(args, 128) < 0) {
        print_err("cp: Could not get arguments.\n");
        return 1;
    }
    
    // Parse "src dest"
    // args is the string passed via Terminal (e.g. "file1.txt file2.txt")
    // Terminal logic for `cp` will pass the substring after "cp ".
    
    if (strlen(args) == 0) {
        print_err("Usage: cp <source> <dest>\n");
        return 1;
    }
    
    char *src = args;
    char *dest = 0;
    
    // Find space
    int i = 0;
    while(args[i]) {
        if (args[i] == ' ') {
            args[i] = 0; // Terminate src
            dest = &args[i+1];
            break;
        }
        i++;
    }
    
    if (!dest) {
        print_err("Usage: cp <source> <dest>\n");
        return 1;
    }
    
    // Skip leading spaces in dest
    while(*dest == ' ') dest++;
    if (*dest == 0) {
        print_err("Usage: cp <source> <dest>\n");
        return 1;
    }

    int fd_in = open(src, 0); // O_RDONLY
    if (fd_in < 0) {
        print_err("cp: Cannot open source file.\n");
        return 1;
    }
    
    // Create dest
    if (creat(dest, 0644) != 0) {
        // creat returns 0 on success in our simplified syscall for now, 
        // or we might need to handle failure if it assumes fd return.
        // My implementation returned 0 on success.
    }
    
    // Re-open dest for writing
    // My open syscall flags: 0=READ, 1=WRITE ? 
    // Need to verify flags in kernel/vfs. 
    // But `vmm` usually ignores. `syscall.c` calls `open_fs`.
    // `fs_open_file` inside `filesystem.c` just marks it open.
    // The `write` syscall checks `fd_table`.
    // Wait, `sys_open` does not take flags actually in `syscall.c` impl?
    // L96: `// int flags = regs->ecx;` (commented out).
    // So generic open is fine.
    
    int fd_out = open(dest, 0); 
    if (fd_out < 0) {
        print_err("cp: Cannot open destination file.\n");
        close(fd_in);
        return 1;
    }
    
    char buf[256];
    int n;
    while( (n = read(fd_in, buf, 256)) > 0 ) {
        write(fd_out, buf, n);
    }
    
    close(fd_in);
    close(fd_out);
    return 0;
}
