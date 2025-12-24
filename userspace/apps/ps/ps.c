#include "../../libc/stdlib.h"

// Helper
void itoa(int n, char *buf) {
    if (n == 0) { buf[0] = '0'; buf[1] = 0; return; }
    int i = 0;
    while (n > 0) {
        buf[i++] = (n % 10) + '0';
        n /= 10;
    }
    buf[i] = 0;
    // Reverse
    for(int j=0; j<i/2; j++) {
        char t = buf[j];
        buf[j] = buf[i-1-j];
        buf[i-1-j] = t;
    }
}

// States
const char *states[] = { "RDY", "RUN", "IO ", "DED" };

int main() {
    print("PID   STATE  NAME\n");
    print("---------------------\n");
    
    process_info_t procs[32];
    int count = get_process_list(procs, 32);
    
    if (count < 0) {
        print("Error getting process list\n");
        return 1;
    }
    
    for(int i=0; i<count; i++) {
        char buf[16];
        
        // PID
        itoa(procs[i].pid, buf);
        print(buf);
        // Padding
        int len = 0; while(buf[len]) len++;
        for(int k=0; k<6-len; k++) print(" ");
        
        // State
        if (procs[i].state >= 0 && procs[i].state <= 3)
            print(states[procs[i].state]);
        else
            print("???");
        print("    ");
        
        // Name
        print(procs[i].name);
        print("\n");
    }
    return 0;
}
