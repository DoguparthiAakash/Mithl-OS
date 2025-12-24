#include "semantic.h"
#include "string.h"
#include "console.h"
#include "memory.h"
#include "process.h"

#define MAX_AGENTS 64

// The Knowledge Base (Agent Registry)
static agent_node_t agent_store[MAX_AGENTS];
static int agent_count = 0;

// Internal Helper: Check if string contains token (case-insensitive-ish)
// We use strstr for now. For a real semantic kernel, this would use embeddings.
static int match_intent(const char *intents, const char *query) {
    if (!intents || !query) return 0;
    
    // Simple substring match
    // e.g. intents="play,game", query="play" -> Match
    // query="game" -> Match
    if (strstr((char*)intents, (char*)query)) return 1;
    
    return 0;
}

void agent_init(void) {
    console_write("[AIOS] Initializing Semantic Agent Core...\n");
    memset(agent_store, 0, sizeof(agent_store));
    agent_count = 0;

    // --- Bootstrap Knowledge (Built-in Agents) ---
    
    // 1. Doom Agent
    agent_register("Doom", "play,game,fps,doom,shooter,fun", "/boot/DOOM1.WAD", 1);
    
    // 2. Shell Agent
    agent_register("Shell", "cmd,command,terminal,cli,run", "internal:shell", 1);
    
    // 3. Settings Agent
    agent_register("Settings", "config,setup,display,resolution,color", "internal:settings", 1);
    
    // 4. File Manager
    agent_register("Files", "file,browse,folder,directory,storage", "internal:files", 1);
    
    // 5. Text Editor
    agent_register("Editor", "write,text,code,edit,note", "internal:editor", 1);
    
    console_write("[AIOS] Semantic Core Ready. Agents: 5\n");
}


int agent_register(const char *name, const char *intents, const char *binary, int trust) {
    if (agent_count >= MAX_AGENTS) {
        console_write("[AIOS] Registry Full!\n");
        return -1;
    }
    
    // Find free slot
    int idx = -1;
    for (int i = 0; i < MAX_AGENTS; i++) {
        if (agent_store[i].active == 0) {
            idx = i;
            break;
        }
    }
    
    if (idx == -1) return -1; // Should not happen if count checked
    
    // Store Data
    // Use strncpy to prevent overflow (we need to implement strncpy properly or use strcpy if safe)
    // Assuming strcpy is safe for these short strings for prototype
    strcpy(agent_store[idx].name, name);
    strcpy(agent_store[idx].intents, intents);
    strcpy(agent_store[idx].binary, binary);
    agent_store[idx].trust_level = trust;
    agent_store[idx].active = 1;
    
    agent_count++;
    return idx;
}

int agent_query(const char *query, char *buffer, size_t size) {
    console_write("[AIOS] Querying Knowledge Base: '");
    console_write(query);
    console_write("'\n");

    // Scan all agents for intent match
    // Priority mechanism: First Match (for now)
    
    for (int i = 0; i < MAX_AGENTS; i++) {
        if (agent_store[i].active) {
            // Check Token 1: "play game" -> currently we search the whole string
            // Ideally we tokenize query.
            // Hack: Check if Query appears in Intents OR Intents appear in Query.
            
            // 1. Does User Query contain the tag? e.g. "play doom" contains "doom"
            if (match_intent(query, agent_store[i].name)) {
                 // Strong match on name
                 strncpy(buffer, agent_store[i].binary, size);
                 return 0; 
            }
            
            // 2. Scan tags
            // e.g. user="I want to play". Agent intents="play,game".
            // "play" is in "play,game".
            // We need to tokenize Agent Intents.
            // Simplified: loop over common keywords
            
            // Hack: If query contains "play" and agent has "play"
            if (strstr((char*)query, "play") && strstr(agent_store[i].intents, "play")) {
                 strncpy(buffer, agent_store[i].binary, size);
                 return 0;
            }
            if (strstr((char*)query, "game") && strstr(agent_store[i].intents, "game")) {
                 strncpy(buffer, agent_store[i].binary, size);
                 return 0;
            }
            if (strstr((char*)query, "edit") && strstr(agent_store[i].intents, "edit")) {
                 strncpy(buffer, agent_store[i].binary, size);
                 return 0;
            }
            if (strstr((char*)query, "file") && strstr(agent_store[i].intents, "file")) {
                 strncpy(buffer, agent_store[i].binary, size);
                 return 0;
            }
             if (strstr((char*)query, "browse") && strstr(agent_store[i].intents, "browse")) {
                 strncpy(buffer, agent_store[i].binary, size);
                 return 0;
            }
        }
    }
    
    return -1; // No match
}

int sys_agent_op(int op, void *arg1, void *arg2) {
    if (op == AGENT_OP_QUERY) {
        // arg1 = query string (userspace ptr)
        // arg2 = result buffer (userspace ptr)
        // TODO: Validate pointers!
        
        char *query = (char*)arg1;
        char *result = (char*)arg2;
        
        // For security, copy query to kernel buffer first
        char kquery[128];
        strncpy(kquery, query, 127);
        kquery[127] = 0;
        
        char kresult[128];
        int ret = agent_query(kquery, kresult, 128);
        
        if (ret == 0) {
            strncpy(result, kresult, 128); // Copy back
        }
        return ret;
    }
    
    if (op == AGENT_OP_REGISTER) {
        // Dynamic registration (TODO)
        return -1; // Not allowed for userspace yet
    }
    
    return -1; // Unknown Op
}
