#include "../../libc/stdlib.h"

int main() {
    print("Listing /\n");
    
    int fd = open("/", 0);
    if (fd < 0) {
        print("Error: Could not open / (fd < 0)\n");
        return 1;
    }
    
    dirent_t *d;
    while ((d = readdir(fd)) != 0) {
        print("- ");
        print(d->name);
        print("\n");
        // We could print d->ino too but print() only takes string. need itoa or printf.
    }
    
    close(fd);
    return 0;
}
