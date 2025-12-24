#include "../../libc/stdlib.h"

int strlen(const char *s) {
    int l=0; while(s[l]) l++; return l;
}

int strcmp(const char *s1, const char *s2) {
    while(*s1 && (*s1 == *s2)) { s1++; s2++; }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

int get_int(const char *s) {
    int res = 0;
    while(*s >= '0' && *s <= '9') {
        res = res * 10 + (*s - '0');
        s++;
    }
    return res;
}

// Minimal variable storage
struct { char name[32]; int val; } vars[32];
int var_count = 0;

void set_var(const char *name, int val) {
    for(int i=0; i<var_count; i++) {
        if (strcmp(vars[i].name, name) == 0) {
            vars[i].val = val;
            return;
        }
    }
    // New
    if (var_count < 32) {
        int i=0; while(name[i]) { vars[var_count].name[i] = name[i]; i++; }
        vars[var_count].name[i] = 0;
        vars[var_count].val = val;
        var_count++;
    }
}

int get_var(const char *name) {
    for(int i=0; i<var_count; i++) {
        if (strcmp(vars[i].name, name) == 0) return vars[i].val;
    }
    return 0;
}

void run_script(int fd) {
    char buf[1024];
    int n = read(fd, buf, 1023);
    if (n <= 0) return;
    buf[n] = 0;
    
    char *p = buf;
    char line[128];
    int li = 0;
    
    while(*p) {
        if (*p == '\n' || *p == ';') {
            line[li] = 0;
            // Parse Line
            char *curr = line;
            while(*curr == ' ') curr++; // Skip space
            
            if (curr[0] == 'p' && curr[1] == 'r' && curr[2] == 'i' && curr[3] == 'n' && curr[4] == 't') {
                // print("hello")
                char *start = curr + 5;
                while(*start && *start != '"') start++;
                if (*start == '"') {
                    start++;
                    char *end = start;
                    while(*end && *end != '"') end++;
                    *end = 0;
                    print(start);
                    print("\n");
                }
            }
            else if (curr[0] == 'i' && curr[1] == 'n' && curr[2] == 't') {
                // int x = 5
                char *name = curr + 4;
                while(*name == ' ') name++;
                char *eq = name;
                while(*eq && *eq != '=') eq++;
                if (*eq == '=') {
                    *eq = 0;
                    // Trim name
                    // simplistic parse
                    
                    int val = get_int(eq + 1);
                    set_var(name, val); // name need trimming? tricky in C without libs
                }
            }
            // Add more...
            
            li = 0;
        } else {
            line[li++] = *p;
        }
        p++;
    }
}

int main() {
    print("Mithl-OS Tiny C Compiler (tcc)\n");
    print("------------------------------\n");
    
    char args[128];
    if (get_cmdline(args, 128) < 0) return 1;
    
    // Parse "cc file.c"
    char *fname = args;
    // Skip command 'cc' or filename of elf
    // actually args returned by get_cmdline IS the argument part only?
    // terminal.c: process_create_elf("cc", "/cc.elf", arg); 
    // arg is "file.c". So fname is "file.c".
    
    // Trim spaces
    while(*fname == ' ') fname++;
    if (*fname == 0) {
        print("Usage: cc <file.c>\n");
        return 1;
    }
    
    int fd = open(fname, 0);
    if (fd < 0) {
        print("cc: Could not open source file: "); print(fname); print("\n");
        return 1;
    }
    
    print("Compiling and Running "); print(fname); print("...\n");
    run_script(fd);
    
    close(fd);
    return 0;
}
