#include "../../libc/stdlib.h"

int main() {
    print("Hello from Userspace App!\n");
    print("Creating a window...\n");
    
    create_window("Hello App", 150, 150, 300, 200);
    
    // Loop forever so the window stays open
    while(1) {
        // Spin
    }
    
    return 0;
}
