#include <semantic.h>
#include <string.h>
#include <memory.h>
#include <console.h>

#define MAX_AGENTS 32

static agent_node_t agents[MAX_AGENTS];
static int agent_count = 0;

void agent_init(void) {
    agent_count = 0;
    memset(agents, 0, sizeof(agents));
    console_write("[SEMANTIC] Core Subsystem Initialized.\n");
    
    // Register Default System Agents
    agent_register("System", "core,kernel,control", "internal", 2);
}

int agent_register(const char *name, const char *intents, const char *binary, int trust) {
    if (agent_count >= MAX_AGENTS) {
        console_write("[SEMANTIC] Registry Full!\n");
        return -1;
    }

    agent_node_t *node = &agents[agent_count++];
    strncpy(node->name, name, 63);
    strncpy(node->intents, intents, 127);
    strncpy(node->binary, binary, 127);
    node->trust_level = trust;
    node->active = 1;

    console_write("[SEMANTIC] Registered Agent: ");
    console_write(name);
    console_write("\n");
    
    return 0; // Success
}

int agent_query(const char *query, char *buffer, size_t size) {
    // Naive Implementation: Search for query string inside "intents"
    // TODO: Implement actual semantic/vector similarity/LLM matching
    
    for (int i = 0; i < agent_count; i++) {
        if (agents[i].active && strstr(agents[i].intents, query)) {
            // Found match!
            strncpy(buffer, agents[i].binary, size - 1);
            buffer[size - 1] = 0; // Ensure null term
            return 0;
        }
    }
    
    return -1; // Not found
}

// Unified System Call Handler for Agent SDK
int sys_agent_op(int op, void *arg1, void *arg2) {
    switch(op) {
        case AGENT_OP_REGISTER: {
            // arg1 = agent_node_t struct pointer (from userspace)
            // We need to copy it safely
            agent_node_t *user_node = (agent_node_t*)arg1;
            if (!user_node) return -1;
            
            // Validate pointers (simple bounds check mock)
            // In real OS, use copy_from_user
            
            return agent_register(user_node->name, user_node->intents, user_node->binary, user_node->trust_level);
        }
        case AGENT_OP_QUERY: {
            // arg1 = query text, arg2 = output buffer (binary path)
            char *query = (char*)arg1;
            char *buffer = (char*)arg2;
            
            return agent_query(query, buffer, 128); // Force size for now or pass as arg3
        }
        case AGENT_OP_INTENT: {
             // Execute Intent? 
             // Just Log for now
             console_write("[SEMANTIC] INTENT: ");
             console_write((char*)arg1);
             console_write("\n");
             return 0;
        }
        default:
            return -1;
    }
}
