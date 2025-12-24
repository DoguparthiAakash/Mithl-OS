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
        print_err("mv: Could not get arguments.\n");
        return 1;
    }
    
    if (strlen(args) == 0) {
        print_err("Usage: mv <old> <new>\n");
        return 1;
    }
    
    char *src = args;
    char *dest = 0;
    
    int i = 0;
    while(args[i]) {
        if (args[i] == ' ') {
            args[i] = 0;
            dest = &args[i+1];
            break;
        }
        i++;
    }
    
    if (!dest) {
        print_err("Usage: mv <old> <new>\n");
        return 1;
    }
    
    while(*dest == ' ') dest++;
    
    if (rename(src, dest) == 0) {
        // Success
        return 0;
    } else {
        print_err("mv: Failed to rename/move file.\n");
        return 1;
    }
}
