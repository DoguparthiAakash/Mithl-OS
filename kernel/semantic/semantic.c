#include "semantic.h"
#include "list.h"
#include "string.h"
#include "memory.h"
#include "console.h"

// Internal Agent List
static list_t *agent_list = NULL;

void agent_init(void) {
    agent_list = list_create();
    console_write("[SEMANTIC] Core Initialized. Agent Knowledge Graph Active.\n");
    
    // Register Default System Agents
    agent_register("OS_Terminal", "shell,cli,cmd,command,terminal,run", "/boot/shell.elf", 2); // Userspace Shell
    agent_register("File_Manager", "files,explore,browse,directory,folder,open", "/boot/filemgr.elf", 2);
    agent_register("Doom_Slayer", "game,play,fps,doom,shoot,kill", "doom", 2); // Internal command
    agent_register("Notepad_AI", "text,edit,write,note,code,txt", "/boot/notepad.elf", 2);
    // agent_register("Settings_Bot", "config,setup,system,about,info,settings", "/boot/settings.elf", 2);
}

int agent_register(const char *name, const char *intents, const char *binary, int trust) {
    if (!agent_list) return -1;
    
    agent_node_t *agent = (agent_node_t*)memory_alloc(sizeof(agent_node_t));
    if (!agent) return -1;
    
    strcpy(agent->name, name);
    strcpy(agent->intents, intents);
    strcpy(agent->binary, binary);
    agent->trust_level = trust;
    agent->active = 1;
    
    list_append(agent_list, agent);
    return 0;
}

// Simple Intent Matching
int agent_query(const char *query, char *buffer, size_t size) {
    if (!agent_list || !query || !buffer) return -1;
    
    list_node_t *node = agent_list->head;
    agent_node_t *best_match = NULL;
    int max_score = 0;
    
    while(node) {
        agent_node_t *agent = (agent_node_t*)node->data;
        int score = 0;
        
        // Copy intents to temp buffer for tokenizer
        char ints[128];
        strcpy(ints, agent->intents);
        
        // Manual Tokenization to avoid missing strtok
        int i = 0;
        char *current = ints;
        while(ints[i]) {
             // Find end of current tag
             int start = i;
             while(ints[i] && ints[i] != ',') i++;
             
             if (i > start) {
                 ints[i] = 0; // Null terminate
                 char *tag = &ints[start];
                 if (strstr(query, tag)) {
                     score++;
                 }
                 if (ints[i+1] == 0) break; // End of string
                 i++; // Skip comma
             } else {
                 i++;
             }
        }
        
        if (score > max_score) {
            max_score = score;
            best_match = agent;
        }
        
        node = node->next;
    }
    
    if (best_match && max_score > 0) {
        size_t len = strlen(best_match->binary);
        if (len >= size) return -1;
        strcpy(buffer, best_match->binary);
        return 0;
    }
    
    return -1; // No match
}

// Syscall Handler
int sys_agent_op(int op, void *arg1, void *arg2) {
    if (op == AGENT_OP_REGISTER) {
        agent_node_t *user_agent = (agent_node_t*)arg1;
        // Validate pointer logic needed here for security in real OS
        return agent_register(user_agent->name, user_agent->intents, user_agent->binary, 0);
    }
    else if (op == AGENT_OP_QUERY) {
        return agent_query((const char*)arg1, (char*)arg2, 128);
    }
    
    return -1;
}
