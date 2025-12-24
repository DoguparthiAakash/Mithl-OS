#include <stdlib.h>

int strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++; s2++;
    }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    
    char name[128];
    char text[1024]; // Simple buffer
    
    print("Insert name with .txt extension (eg. name.txt): ");
    gets(name);
    
    // Check forbidden names
    if (strcmp(name, "con.txt") == 0 || strcmp(name, "aux.txt") == 0 || strcmp(name, "prn.txt") == 0 ||
        strcmp(name, "con") == 0 || strcmp(name, "aux") == 0 || strcmp(name, "prn") == 0) {
        print("Can't write to file\n");
        return 1;
    }
    
    int fd = creat(name, 0);
    if (fd < 0) {
        print("Error creating file.\n");
        return 1;
    }
    
    print("Insert text down here, when finished press enter:\n");
    gets(text);
    
    write(fd, text, 0); // Need strlen. Wait, stdlib should have strlen?
    // I need to implement strlen locally if not in stdlib shim or include string.h wrapper
    // user libc usually has string.h?
    // Let's implement helper or use loop.
    int len = 0; while(text[len]) len++;
    write(fd, text, len);
    
    close(fd);
    
    print("File saved.\n");
    return 0;
}
