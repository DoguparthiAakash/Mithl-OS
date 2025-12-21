#include "vfs.h"
#include "string.h"
#include "console.h"
#include "memory.h"
#include "list.h" 

// --- RamFS Structures ---

// Since we lack a complex allocator or strict struct, we'll implement a simple tree
// Any "fs_node_t" of type DIRECTORY will rely on `impl` to point to a list of children.
// RAMFS specific node extension
typedef struct ramfs_node {
    const char *name;
    void *data; // For file: content buffer. For dir: list_t* of children
    uint32_t size;
    uint32_t flags;
    struct ramfs_node *parent;
} ramfs_node_t;

// We need to map generic fs_node_t back to our ramfs_node_t to access data.
// For this simple impl, we can misuse the 'impl' field of fs_node_t to store the pointer to our internal ramfs structure?
// Or just keep it simple: fs_node_t IS the structure we use, with some custom internal management.
// Actually, generic vfs uses fs_node_t which is allocated by the fs driver.
// So we can allocate fs_node_t + extra data, or just use fs_node_t and 'ptr' or 'impl'.

// Let's use strict fs_node_t creation.

struct dirent dir_entry;

// -- Prototypes --
fs_node_t *ramfs_create_file(const char *name, const char *content);
fs_node_t *ramfs_create_dir(const char *name);
void ramfs_add_child(fs_node_t *dir, fs_node_t *child);

// -- Handlers --

uint32_t ramfs_read(fs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer) {
    if ((node->flags & FS_FILE) != FS_FILE) return 0;
    
    // Node->impl is pointer to data buffer for files in this simple FS
    char *data = (char*)node->impl;
    if (offset > node->length) return 0;
    if (offset + size > node->length) size = node->length - offset;
    
    memcpy(buffer, data + offset, size);
    return size;
}

struct dirent *ramfs_readdir(fs_node_t *node, uint32_t index) {
    if ((node->flags & FS_DIRECTORY) != FS_DIRECTORY) return 0;
    
    // Node->ptr is pointer to list_t of children
    list_t *children = (list_t*)node->ptr;
    if (!children) return 0;
    
    list_node_t *item = children->head;
    uint32_t i = 0;
    while (item) {
        if (i == index) {
            fs_node_t *child = (fs_node_t*)item->data;
            strcpy(dir_entry.name, child->name);
            dir_entry.ino = child->inode;
            return &dir_entry;
        }
        item = item->next;
        i++;
    }
    return 0;
}

fs_node_t *ramfs_finddir(fs_node_t *node, char *name) {
    if ((node->flags & FS_DIRECTORY) != FS_DIRECTORY) return 0;
    
    list_t *children = (list_t*)node->ptr;
    if (!children) return 0;
    
    list_node_t *item = children->head;
    while (item) {
        fs_node_t *child = (fs_node_t*)item->data;
        if (strcmp(child->name, name) == 0) {
            return child;
        }
        item = item->next;
    }
    return 0;
}

// -- Creation --

// NEW: Write support
uint32_t ramfs_write(fs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer) {
    if ((node->flags & FS_FILE) != FS_FILE) return 0;
    
    // Check if we need to expand
    uint32_t new_end = offset + size;
    if (new_end > node->length) {
        // Simple realloc: Alloc new, Copy old, Free old
        // Note: memory_realloc would be nice
        char *new_buf = (char*)memory_alloc(new_end + 1);
        char *old_buf = (char*)node->impl;
        
        if (old_buf) {
            if (node->length > 0) memcpy(new_buf, old_buf, node->length);
            memory_free(old_buf);
        }
        node->impl = (uint32_t)new_buf;
        node->length = new_end;
    }
    
    char *data = (char*)node->impl;
    memcpy(data + offset, buffer, size);
    data[node->length] = 0; // Null terminate for safety (text files)
    
    return size;
}

// NEW: Unlink (Delete)
void ramfs_vfs_unlink(fs_node_t *parent, char *name) {
    if ((parent->flags & FS_DIRECTORY) != FS_DIRECTORY) return;
    
    list_t *children = (list_t*)parent->ptr;
    if (!children) return;
    
    // Find child node
    list_node_t *item = children->head;
    int index = 0;
    while (item) {
        fs_node_t *child = (fs_node_t*)item->data;
        if (strcmp(child->name, name) == 0) {
            // Found it. Remove from list.
            list_remove_node(children, item);
            
            // Clean up child resources (simple version)
            // If file, free buffer
            if ((child->flags & FS_FILE) == FS_FILE) {
                if (child->impl) memory_free((void*)child->impl);
            }
            // If dir, we should recursively delete? For now, we allow leaking or assume empty.
            // In a real OS, we'd fail if Dir not empty. 
            // We'll just free the node struct.
            
            memory_free(child);
            return;
        }
        item = item->next;
        index++;
    }
}

// -- Creation --

fs_node_t *allocate_node(const char *name, uint32_t flags) {
    fs_node_t *node = (fs_node_t*)memory_alloc(sizeof(fs_node_t));
    memset(node, 0, sizeof(fs_node_t));
    strcpy(node->name, name);
    node->flags = flags;
    node->read = ramfs_read;
    node->write = ramfs_write; // Bind write
    node->readdir = ramfs_readdir;
    node->finddir = ramfs_finddir;
    node->create = 0; // Set later if directory
    node->mkdir = 0;  // Set later if directory
    node->unlink = 0; // Set later
    return node;
}

fs_node_t *ramfs_create_file(const char *name, const char *content) {
    fs_node_t *node = allocate_node(name, FS_FILE);
    uint32_t len = strlen(content);
    char *buf = (char*)memory_alloc(len + 1);
    strcpy(buf, content);
    
    node->length = len;
    node->impl = (uint32_t)buf; // Store data pointer in impl
    return node;
}

void ramfs_vfs_create(fs_node_t *parent, char *name, uint16_t permission);
void ramfs_vfs_mkdir(fs_node_t *parent, char *name, uint16_t permission);

fs_node_t *ramfs_create_dir(const char *name) {
    fs_node_t *node = allocate_node(name, FS_DIRECTORY);
    node->ptr = (struct fs_node*)list_create();
    node->create = ramfs_vfs_create;
    node->mkdir = ramfs_vfs_mkdir;
    node->unlink = ramfs_vfs_unlink; // Bind unlink
    return node;
}

void ramfs_add_child(fs_node_t *dir, fs_node_t *child) {
    list_t *l = (list_t*)dir->ptr;
    list_append(l, child);
    // generic fs_node doesn't have parent
}

void ramfs_vfs_create(fs_node_t *parent, char *name, uint16_t permission) {
    (void)permission;
    fs_node_t *file = ramfs_create_file(name, "");
    ramfs_add_child(parent, file);
}

void ramfs_vfs_mkdir(fs_node_t *parent, char *name, uint16_t permission) {
    (void)permission;
    fs_node_t *dir = ramfs_create_dir(name);
    ramfs_add_child(parent, dir);
}

// -- Init --

// Initialise Standard Unix Paths for Debian Compatibility
void ramfs_init(void) {
    fs_root = ramfs_create_dir("/");
    
    // 1. Core Directories
    fs_node_t *bin = ramfs_create_dir("bin");
    fs_node_t *usr = ramfs_create_dir("usr");
    fs_node_t *dev = ramfs_create_dir("dev");
    fs_node_t *etc = ramfs_create_dir("etc");
    fs_node_t *lib = ramfs_create_dir("lib");
    fs_node_t *tmp = ramfs_create_dir("tmp");
    fs_node_t *home = ramfs_create_dir("home");
    
    ramfs_add_child(fs_root, bin);
    ramfs_add_child(fs_root, usr);
    ramfs_add_child(fs_root, dev);
    ramfs_add_child(fs_root, etc);
    ramfs_add_child(fs_root, lib);
    ramfs_add_child(fs_root, tmp);
    ramfs_add_child(fs_root, home);
    
    // 2. /usr subdirectories
    ramfs_add_child(usr, ramfs_create_dir("bin"));
    ramfs_add_child(usr, ramfs_create_dir("lib"));
    ramfs_add_child(usr, ramfs_create_dir("share"));
    
    // 3. User Home
    fs_node_t *aakash = ramfs_create_dir("aakash");
    ramfs_add_child(home, aakash);
    
    ramfs_add_child(aakash, ramfs_create_dir("Documents"));
    ramfs_add_child(aakash, ramfs_create_dir("Downloads"));
    ramfs_add_child(aakash, ramfs_create_dir("Desktop"));
    
    // 4. Config Files
    ramfs_add_child(etc, ramfs_create_file("hostname", "mithl-os\n"));
    ramfs_add_child(etc, ramfs_create_file("passwd", "root:x:0:0:root:/root:/bin/bash\naakash:x:1000:1000:aakash:/home/aakash:/bin/bash\n"));
    
    // 5. Welcome File
    ramfs_add_child(aakash, ramfs_create_file("welcome.txt", "Welcome to Mithl OS! Linux binary compatibility layer active.\n"));
}

// Legacy function removed/merged
void ramfs_init_clean(void) {
    // Redirect
    ramfs_init();
}
#include "boot_info.h"

// Load Multiboot Modules into / (root)
void ramfs_load_modules(boot_info_t *info) {
    if (!info) return;
    
    console_write("[RAMFS] Loading Modules...\n");
    
    for (uint32_t i = 0; i < info->mod_count; i++) {
        boot_module_t *mod = &info->modules[i];
        
        char *name = mod->string;
        if (!name || !*name) name = "module.bin";
        
        // Extract filename from path if needed (e.g. /modules/doom.wad -> doom.wad)
        char *basename = name;
        for (char *c = name; *c; c++) {
            if (*c == '/') basename = c + 1;
        }
        
        // Create file
        uint32_t size = mod->mod_end - mod->mod_start;
        // Optimization: Don't copy, just point to module location? 
        // For simplicity/safety, we COPY because modules might be reclaimed if we use PMM poorly.
        // But our PMM is dumb, so it might mark used.
        // Let's alloc and copy to be safe in `ramfs_create_file`.
        // Wait, `ramfs_create_file` takes a string content. We need binary support.
        
        fs_node_t *node = allocate_node(basename, FS_FILE);
        char *buf = (char*)memory_alloc(size);
        memcpy(buf, (void*)mod->mod_start, size);
        
        node->length = size;
        node->impl = (uint32_t)buf;
        
        ramfs_add_child(fs_root, node);
        
        console_write("  Loaded: ");
        console_write(basename);
        console_write("\n");
    }
}

// --- Persistence Logic ---

#define BACKUP_MAGIC 0x424B5550 // "BKUP"
#define SECTOR_SIZE 512

// ATA Externs
extern int ata_read_sector(uint32_t lba, uint8_t *buffer);
extern int ata_write_sector(uint32_t lba, const uint8_t *buffer);

typedef struct {
    uint32_t magic;
    uint32_t count;
} backup_header_t;

typedef struct {
    char path[64];
    uint32_t size;
    uint32_t sector_offset; 
} backup_entry_t;

// Helper to traverse and collect files
void collect_files(fs_node_t *node, char *path_prefix, backup_entry_t *entries, uint32_t *count, uint32_t *data_sector_ptr) {
    if (!node) return;
    
    // Construct full path
    char full_path[128];
    if (strcmp(path_prefix, "/") == 0) strcpy(full_path, "/");
    else {
        strcpy(full_path, path_prefix);
        if (full_path[strlen(full_path)-1] != '/') strcat(full_path, "/");
    }
    strcat(full_path, node->name);
    
    // If file, add to list
    if ((node->flags & 0x01) == 0x01) { // FS_FILE
        if (*count >= 100) return; // Limit 100 files for demo
        
        backup_entry_t *ent = &entries[*count];
        strcpy(ent->path, full_path);
        ent->size = node->length;
        ent->sector_offset = *data_sector_ptr;
        
        // Write Data immediately
        uint32_t sectors_needed = (node->length + SECTOR_SIZE - 1) / SECTOR_SIZE;
        
        char *file_data = (char*)node->impl; // Direct access
        if (file_data) {
             for (uint32_t s = 0; s < sectors_needed; s++) {
                 uint8_t sec_buf[SECTOR_SIZE];
                 memset(sec_buf, 0, SECTOR_SIZE);
                 
                 uint32_t chunk = node->length - (s * SECTOR_SIZE);
                 if (chunk > SECTOR_SIZE) chunk = SECTOR_SIZE;
                 
                 memcpy(sec_buf, file_data + (s * SECTOR_SIZE), chunk);
                 ata_write_sector(*data_sector_ptr + s, sec_buf);
             }
        }
        *data_sector_ptr += sectors_needed;
        (*count)++;
    }
    
    // If dir, recurse
    if ((node->flags & 0x02) == 0x02) { // FS_DIRECTORY
        list_t *children = (list_t*)node->ptr;
        if (children) {
            list_node_t *item = children->head;
            while (item) {
                collect_files((fs_node_t*)item->data, full_path, entries, count, data_sector_ptr);
                item = item->next;
            }
        }
    }
}

void ramfs_backup(void) {
    console_write("[BACKUP] Starting Backup to Disk (ZFS Snapshot)...\n");
    
    backup_entry_t entries[100];
    uint32_t file_count = 0;
    
    // Data starts after Header (Sec 0) + Metadata (Sec 1..15)
    uint32_t data_start_sector = 16;
    
    // Traverse /home/aakash (User data only)
    fs_node_t *home = vfs_resolve_path("/home/aakash");
    collect_files(home, "/home/aakash", entries, &file_count, &data_start_sector);
    
    // Write Header
    backup_header_t header;
    header.magic = BACKUP_MAGIC;
    header.count = file_count;
    
    uint8_t sec0[SECTOR_SIZE];
    memset(sec0, 0, SECTOR_SIZE);
    memcpy(sec0, &header, sizeof(header));
    ata_write_sector(0, sec0);
    
    // Write Metadata
    uint8_t *entry_bytes = (uint8_t*)entries;
    for (int i = 0; i < 15; i++) {
        uint8_t buf[SECTOR_SIZE];
        memcpy(buf, entry_bytes + (i * SECTOR_SIZE), SECTOR_SIZE);
        ata_write_sector(1 + i, buf);
    }
    
    console_write("[BACKUP] Completed successfully.\n");
}
