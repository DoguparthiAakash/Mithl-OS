#include "../../libc/stdlib.h"

// Simple string helpers
int strlen(const char *s) {
    int l=0; while(s[l]) l++; return l;
}

int strcmp(const char *s1, const char *s2) {
    while(*s1 && (*s1 == *s2)) { s1++; s2++; }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

void print_err(const char *s) {
    // Write to stderr (FD 2)
    // using generic print for now
    print(s); // print uses stdout/sys_write
}

int main() {
    char cmdline[128];
    if (get_cmdline(cmdline, 128) < 0) {
        print_err("cat: Could not get arguments.\n");
        return 1;
    }
    
    // Parse arguments
    // cmdline might be just arguments? Or whole line?
    // "args" passed to process_create_elf.
    // If we passed "", it's empty.
    
    if (strlen(cmdline) == 0) {
        print_err("Usage: cat <filename>\n");
        return 1;
    }
    
    // Process arguments (simple split by space)
    char *filename = cmdline;
    // Skip leading spaces
    while(*filename == ' ') filename++;
    
    if (*filename == 0) {
        print_err("Usage: cat <filename>\n");
        return 1;
    }
    
    int fd = open(filename, 0); // O_RDONLY
    if (fd < 0) {
        print_err("cat: ");
        print_err(filename);
        print_err(": No such file or directory\n");
        return 1;
    }
    
    char buf[128];
    int n;
    while(1) {
        n = read(fd, buf, 127);
        if (n <= 0) break;
        buf[n] = 0;
        print(buf);
    }
    
    close(fd);
    return 0;
}
