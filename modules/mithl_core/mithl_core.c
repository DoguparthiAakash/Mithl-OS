#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/string.h>

#define DRIVER_NAME "mithl_core"
#define MAX_AGENTS 64

// --- Structures ---
typedef struct {
    char name[64];
    char intents[128];
    char binary[128];
    int  trust_level;
    int  active;
} agent_node_t;

static agent_node_t agent_store[MAX_AGENTS];
static int agent_count = 0;

// --- Helper Logic (Ported from semantic.c) ---
static int match_intent(const char *intents, const char *query) {
    if (!intents || !query) return 0;
    // strstr is available in Linux Kernel
    if (strstr((char*)intents, (char*)query)) return 1;
    return 0;
}

static int agent_register(const char *name, const char *intents, const char *binary, int trust) {
    if (agent_count >= MAX_AGENTS) return -1;
    
    int idx = agent_count; // Simplified for LKM
    
    strncpy(agent_store[idx].name, name, 63);
    strncpy(agent_store[idx].intents, intents, 127);
    strncpy(agent_store[idx].binary, binary, 127);
    agent_store[idx].trust_level = trust;
    agent_store[idx].active = 1;
    
    agent_count++;
    printk(KERN_INFO "[Mithl] Registered Agent: %s\n", name);
    return idx;
}

static int agent_query_kernel(const char *query, char *buffer, size_t size) {
    printk(KERN_INFO "[Mithl] Query: %s\n", query);
    
    for (int i = 0; i < agent_count; i++) {
        if (agent_store[i].active) {
            // Match Logic Ported
            if (match_intent(query, agent_store[i].name)) {
                 strncpy(buffer, agent_store[i].binary, size);
                 return 0; 
            }
            if (strstr((char*)query, "play") && strstr(agent_store[i].intents, "play")) {
                 strncpy(buffer, agent_store[i].binary, size);
                 return 0;
            }
             if (strstr((char*)query, "game") && strstr(agent_store[i].intents, "game")) {
                 strncpy(buffer, agent_store[i].binary, size);
                 return 0;
            }
            // Add more keywords as needed
            if (strstr((char*)query, "terminal") && strstr(agent_store[i].intents, "terminal")) {
                 strncpy(buffer, agent_store[i].binary, size);
                 return 0;
            }
        }
    }
    return -1;
}

// --- IOCTL Interface ---
#define MITHL_IOC_MAGIC 'M'
#define MITHL_IOC_QUERY  _IOWR(MITHL_IOC_MAGIC, 1, char[256])

static long mithl_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
    char kbuffer[256];
    char result[128];
    int ret = 0;
    
    switch(cmd) {
        case MITHL_IOC_QUERY:
            if (copy_from_user(kbuffer, (char __user *)arg, 256)) return -EFAULT;
            
            // Assume input format: "query_string"
            // We'll use the same buffer for result? Or struct?
            // For simplicity: Input=Query, Output=ResultString
            
            ret = agent_query_kernel(kbuffer, result, 128);
            if (ret == 0) {
                 if (copy_to_user((char __user *)arg, result, 128)) return -EFAULT;
            } else {
                 if (copy_to_user((char __user *)arg, "NOT FOUND", 10)) return -EFAULT;
            }
            break;
            
        default:
            return -EINVAL;
    }
    return 0;
}

static const struct file_operations mithl_fops = {
    .owner = THIS_MODULE,
    .unlocked_ioctl = mithl_ioctl,
};

static struct miscdevice mithl_device = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "mithl",
    .fops = &mithl_fops,
};

static int __init mithl_init(void) {
    printk(KERN_INFO "[Mithl] Semantic Core Loading...\n");
    
    // Bootstrap Agents
    agent_register("Doom", "play,game,fps,doom", "/usr/bin/doom", 1);
    agent_register("Shell", "terminal,cmd,cli", "/bin/sh", 1);
    agent_register("Settings", "config,setup", "/usr/bin/settings", 1);

    int error = misc_register(&mithl_device);
    if (error) {
        printk(KERN_ERR "[Mithl] Failed to register device\n");
        return error;
    }
    
    printk(KERN_INFO "[Mithl] Semantic Core Ready. Device: /dev/mithl\n");
    return 0;
}

static void __exit mithl_exit(void) {
    misc_deregister(&mithl_device);
    printk(KERN_INFO "[Mithl] Core Unloaded.\n");
}

module_init(mithl_init);
module_exit(mithl_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mithl-OS Team");
MODULE_DESCRIPTION("Mithl-OS Semantic Agent Core");
