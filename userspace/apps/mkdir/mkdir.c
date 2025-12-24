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
        print_err("mkdir: Could not get arguments.\n");
        return 1;
    }
    
    if (strlen(args) == 0) {
        print_err("Usage: mkdir <directory>\n");
        return 1;
    }
    
    // Create Mode 0755 (default)
    // In our simplified FS, permissions might be ignored or basic.
    if (mkdir(args, 0755) == 0) {
        print("Directory created: ");
        print(args);
        print("\n");
        return 0;
    } else {
        print_err("mkdir: Failed to create directory. (Already exists or invalid path)\n");
        return 1;
    }
}
