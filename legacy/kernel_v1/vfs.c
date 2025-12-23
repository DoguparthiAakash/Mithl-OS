#include "vfs.h"
#include "string.h"
#include "memory.h"

fs_node_t *fs_root = 0;

uint32_t read_fs(fs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer)
{
    if (node->read != 0)
        return node->read(node, offset, size, buffer);
    else
        return 0;
}

uint32_t write_fs(fs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer)
{
    if (node->write != 0)
        return node->write(node, offset, size, buffer);
    else
        return 0;
}

void open_fs(fs_node_t *node, uint8_t read, uint8_t write)
{
    if (node->open != 0)
        node->open(node);
}

void close_fs(fs_node_t *node)
{
    if (node->close != 0)
        node->close(node);
}

struct dirent *readdir_fs(fs_node_t *node, uint32_t index)
{
    if ((node->flags & 0x7) == FS_DIRECTORY && node->readdir != 0)
        return node->readdir(node, index);
    else
        return 0;
}

void create_fs(fs_node_t *parent, char *name, uint16_t permission) {
    if (parent->create != 0)
        parent->create(parent, name, permission);
}

void mkdir_fs(fs_node_t *parent, char *name, uint16_t permission) {
    if (parent->mkdir != 0)
        parent->mkdir(parent, name, permission);
}

void unlink_fs(fs_node_t *parent, char *name) {
    if (parent->unlink != 0)
        parent->unlink(parent, name);
}

fs_node_t *finddir_fs(fs_node_t *node, char *name)
{
    if ((node->flags & 0x7) == FS_DIRECTORY && node->finddir != 0)
        return node->finddir(node, name);
    else
        return 0;
}

void vfs_init(void) {
    // Initialized by RamFS or specific FS driver
}

// Helper: Resolve path to node
fs_node_t *vfs_resolve_path(const char *path) {
    if (!path || !fs_root) return 0;
    
    fs_node_t *current = fs_root;
    char path_buf[256];
    
    // Handle absolute/relative basics (simplified: always assume absolute for now or handle /)
    if (path[0] == '/') {
        current = fs_root;
        // copy path skipping leading /
        strcpy(path_buf, path + 1);
    } else {
        // Relative path support would require cwd context, which vfs.c doesn't track.
        // For now, the caller (terminal) should provide absolute path or we assume root-relative if no slash.
        // Ideally terminal expands cwd + path.
        strcpy(path_buf, path);
    }
    
    // Split by /
    // Since we don't have strtok_r, we do manual parsing
    char *p = path_buf;
    char *segment = p;
    
    while (*p) {
        if (*p == '/') {
            *p = 0; // Terminate segment
            
            if (strlen(segment) > 0) {
                fs_node_t *next = finddir_fs(current, segment);
                if (!next) return 0; // Not found
                current = next;
            }
            
            segment = p + 1;
        }
        p++;
    }
    
    // Last segment
    if (strlen(segment) > 0) {
        fs_node_t *next = finddir_fs(current, segment);
        if (!next) return 0;
        current = next;
    }
    
    return current;
}

// Delete file or folder (recursive for directories)
int vfs_delete(fs_node_t *parent, const char *name) {
    if (!parent || !name) return -1;
    
    // Find the node to delete
    fs_node_t *node = finddir_fs(parent, (char*)name);
    if (!node) return -1;  // Not found
    
    // If it's a directory, recursively delete contents first
    if ((node->flags & 0x7) == FS_DIRECTORY) {
        uint32_t index = 0;
        while (1) {
            struct dirent *entry = readdir_fs(node, index);
            if (!entry) break;
            
            // Skip . and ..
            if (strcmp(entry->name, ".") == 0 || strcmp(entry->name, "..") == 0) {
                index++;
                continue;
            }
            
            // Recursively delete child
            vfs_delete(node, entry->name);
            // Don't increment index - after deletion, next item shifts down
        }
    }
    
    // Now delete the node itself using unlink
    unlink_fs(parent, (char*)name);
    return 0;
}

// Rename file or folder
int vfs_rename(fs_node_t *parent, const char *old_name, const char *new_name) {
    if (!parent || !old_name || !new_name) return -1;
    
    // Find the node
    fs_node_t *node = finddir_fs(parent, (char*)old_name);
    if (!node) return -1;
    
    // Check if new name already exists
    fs_node_t *existing = finddir_fs(parent, (char*)new_name);
    if (existing) return -1;  // Name collision
    
    // Simply update the name field
    strncpy(node->name, new_name, 127);
    node->name[127] = 0;
    
    return 0;
}

// Copy file or folder
int vfs_copy(const char *src_path, const char *dest_path) {
    if (!src_path || !dest_path) return -1;
    
    fs_node_t *src = vfs_resolve_path(src_path);
    if (!src) return -1;
    
    // Extract destination directory and filename
    char dest_dir[256];
    char dest_name[128];
    strcpy(dest_dir, dest_path);
    
    char *last_slash = NULL;
    for (char *p = dest_dir; *p; p++) {
        if (*p == '/') last_slash = p;
    }
    
    if (last_slash) {
        strcpy(dest_name, last_slash + 1);
        *last_slash = 0;
    } else {
        strcpy(dest_name, dest_path);
        strcpy(dest_dir, "/");
    }
    
    fs_node_t *dest_parent = vfs_resolve_path(dest_dir);
    if (!dest_parent) return -1;
    
    // Check if it's a file or directory
    if ((src->flags & 0x7) == FS_FILE) {
        // Create new file
        create_fs(dest_parent, dest_name, 0644);
        
        // Copy contents
        fs_node_t *dest = finddir_fs(dest_parent, dest_name);
        if (dest && src->length > 0) {
            uint8_t *buffer = (uint8_t*)memory_alloc(src->length);
            if (buffer) {
                read_fs(src, 0, src->length, buffer);
                write_fs(dest, 0, src->length, buffer);
                memory_free(buffer);
            }
        }
    } else if ((src->flags & 0x7) == FS_DIRECTORY) {
        // Create new directory
        mkdir_fs(dest_parent, dest_name, 0755);
        
        // Recursively copy contents
        fs_node_t *dest_dir_node = finddir_fs(dest_parent, dest_name);
        if (dest_dir_node) {
            uint32_t index = 0;
            while (1) {
                struct dirent *entry = readdir_fs(src, index++);
                if (!entry) break;
                
                if (strcmp(entry->name, ".") == 0 || strcmp(entry->name, "..") == 0)
                    continue;
                
                // Build paths for recursive copy
                char src_child[256];
                char dest_child[256];
                strcpy(src_child, src_path);
                strcat(src_child, "/");
                strcat(src_child, entry->name);
                
                strcpy(dest_child, dest_path);
                strcat(dest_child, "/");
                strcat(dest_child, entry->name);
                
                vfs_copy(src_child, dest_child);
            }
        }
    }
    
    return 0;
}

// Move file or folder (copy + delete)
int vfs_move(const char *src_path, const char *dest_path) {
    if (!src_path || !dest_path) return -1;
    
    // First copy
    if (vfs_copy(src_path, dest_path) != 0) {
        return -1;
    }
    
    // Then delete source
    // Extract parent directory and filename from source
    char src_dir[256];
    char src_name[128];
    strcpy(src_dir, src_path);
    
    char *last_slash = NULL;
    for (char *p = src_dir; *p; p++) {
        if (*p == '/') last_slash = p;
    }
    
    if (last_slash) {
        strcpy(src_name, last_slash + 1);
        *last_slash = 0;
    } else {
        strcpy(src_name, src_path);
        strcpy(src_dir, "/");
    }
    
    fs_node_t *src_parent = vfs_resolve_path(src_dir);
    if (src_parent) {
        vfs_delete(src_parent, src_name);
    }
    
    return 0;
}
