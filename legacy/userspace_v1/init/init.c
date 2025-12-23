
#include "../libc/syscall.h"

// Explicit syscall wrapper since we don't have full libc linked properly yet in test app
// or we rely on stdlib.c

int main(int argc, char **argv) {
    // print("Mithl-OS Userspace Init Started!\n");
    // Directly use syscall for basic output if print not avail
    // sys_write(stdout, msg, len)
    // 4 = SYS_WRITE
    const char *msg = "Hello from Userspace Init!\n";
    syscall_3(4, 1, (int)msg, 27);
    
    // Test Fork/Exec concept
    int pid = syscall_0(2); // SYS_FORK
    
    if (pid == 0) {
        // Child
        const char *msg2 = "Hello from Child!\n";
        syscall_3(4, 1, (int)msg2, 18);
        syscall_1(1, 42); // Exit(42)
    } else {
        // Parent
        const char *msg3 = "Parent Waiting...\n";
        syscall_3(4, 1, (int)msg3, 18);
        
        int status = 0;
        syscall_3(7, pid, (int)&status, 0); // SYS_WAITPID
        
        const char *msg4 = "Child Exited. Parent Exiting.\n";
        syscall_3(4, 1, (int)msg4, 30);
    }
    
    syscall_1(1, 0); // Exit(0)
    return 0;
}
