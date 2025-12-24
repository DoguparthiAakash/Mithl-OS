#ifndef MITHL_SEMANTIC_H
#define MITHL_SEMANTIC_H

#include <stdint.h>
#include <stddef.h>

// --- Data Structures ---

// Represents an Intelligent Agent in the system
// Corresponds to AIOS Architecture "Agent"
typedef struct {
    char name[64];      // Unique Name (e.g. "Doom-Agent")
    char intents[128];  // Comma-separated tags (e.g. "play,game,fps,action")
    char binary[128];   // Path to executable or Internal Handler ID
    int  trust_level;   // 0=User, 1=System, 2=Kernel
    int  active;        // 1=Registered, 0=Free
} agent_node_t;

// --- Syscall Operations ---

#define AGENT_OP_REGISTER 1
#define AGENT_OP_QUERY    2  // Find an agent that can handle X
#define AGENT_OP_INTENT   3  // Execute intent X
#define AGENT_OP_LEARN    4  // Teach kernel new knowledge

// --- Kernel API ---

// Initialize the Semantic Core Subsystem
void agent_init(void);

// Register a new agent (Kernel Internal or via Syscall)
int agent_register(const char *name, const char *intents, const char *binary, int trust);

// Find an agent capable of handling the query.
// Writes the agent's binary path/action to 'buffer'.
// Returns 0 on success, <0 on failure.
int agent_query(const char *query, char *buffer, size_t size);

// Unified System Call Handler for Agent SDK
// op: Operation Code
// arg1: Parameter 1 (usually pointer to struct or string)
// arg2: Parameter 2 (usually length or secondary specific arg)
int sys_agent_op(int op, void *arg1, void *arg2);

#endif // MITHL_SEMANTIC_H
