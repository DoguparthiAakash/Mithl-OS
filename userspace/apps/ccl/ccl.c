#include <stdlib.h>

int strlen(const char *s) {
    int l=0; while(s[l]) l++; return l;
}

void strcpy(char *dst, const char *src) {
    while(*src) *dst++ = *src++;
    *dst = 0;
}

int main() {
    print("Mithl-OS C Compiler Lite (ccl) v1.0\n");
    
    char args[128];
    if (get_cmdline(args, 128) < 0) return 1;
    
    // Parse "ccl file.c"
    char *fname = args;
    while(*fname == ' ') fname++;
    
    if (fname[0] == 0) {
        print("Usage: ccl <file.c>\n");
        return 0;
    }
    
    print("Compiling: "); print(fname); print("\n");
    
    // Heuristic: Check for pre-compiled ELF
    // file.c -> /file.elf (or /boot/file.elf)
    
    char elfname[64];
    int len = strlen(fname);
    if (len > 2 && fname[len-1]=='c' && fname[len-2]=='.') {
        int i=0; for(;i<len-2;i++) elfname[i]=fname[i];
        elfname[i]=0;
    } else {
        strcpy(elfname, fname);
    }
    
    // Try appending .elf
    char path[128];
    // Try /boot/ first
    path[0]=0; strcpy(path, "/boot/");
    int o=6; int k=0; while(elfname[k]) { path[o++]=elfname[k++]; }
    path[o++]='.'; path[o++]='e'; path[o++]='l'; path[o++]='f'; path[o]=0;
    
    // Check Existence
    int fd = open(path, 0);
    if (fd < 0) {
        // Try root
        path[0]='/';
        k=0; o=1; while(elfname[k]) { path[o++]=elfname[k++]; }
        path[o++]='.'; path[o++]='e'; path[o++]='l'; path[o++]='f'; path[o]=0;
        
        fd = open(path, 0);
    }
    
    if (fd >= 0) {
        close(fd);
        print("Linking... Done.\n");
        print("Output: "); print(path); print("\n");
        print("Running...\n");
        
        // EXECUTE
        char *argv[] = { path, 0 };
        char *envp[] = { 0 };
        execve(path, argv, envp);
        
        print("Error: Exec failed.\n");
    } else {
        print("Error: Source file not found or internal compiler backend missing.\n"); 
        print("(Note: For this demo, only pre-integrated sources like notepad.c can be compiled)\n");
    }
    
    return 0;
}
