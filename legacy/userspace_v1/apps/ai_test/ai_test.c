#define SYS_AGENT_OP 200
#define AGENT_OP_QUERY 2

// Minimal LibC replacement for this test app
void print(const char *str) {
    // Syscall 4 = write (Linux compatible or wrappers)
    // But we don't have wrappers yet.
    // Use SYS_MITHL_LOG (102) strictly for now to be safe
    // syscall(102, str, 0)
    
    __asm__ volatile (
        "int $0x80"
        : 
        : "a"(102), "b"(str)
    );
}

void exit(int code) {
    __asm__ volatile (
        "int $0x80"
        : 
        : "a"(1), "b"(code)
    );
    while(1);
}

int query_agent(const char *query, char *result) {
    int ret;
    __asm__ volatile (
        "int $0x80"
        : "=a"(ret)
        : "a"(SYS_AGENT_OP), "b"(AGENT_OP_QUERY), "c"(query), "d"(result)
    );
    return ret;
}

void _start() {
    print("\n--- Semantic Agent Test ---\n");
    
    char buffer[128];
    
    // Test 1: Query for Game
    print("Query: 'play game'\n");
    int ret = query_agent("play game", buffer);
    if (ret == 0) {
        print("Result: ");
        print(buffer);
        print("\n");
    } else {
        print("Result: NOT FOUND\n");
    }
    
    // Test 2: Query for Shell
    print("Query: 'open terminal'\n");
    ret = query_agent("open terminal", buffer);
    if (ret == 0) {
        print("Result: ");
        print(buffer);
        print("\n");
    } else {
        print("Result: NOT FOUND\n");
    }
    
    print("--- Test Complete ---\n");
    exit(0);
}
