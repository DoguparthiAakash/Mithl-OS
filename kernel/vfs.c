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
                fs_node_t *msg = finddir_fs(current, segment);
                if (!msg) return 0; // Not found
                current = msg;
            }
            
            segment = p + 1;
        }
        p++;
    }
    
    // Last segment
    if (strlen(segment) > 0) {
        fs_node_t *msg = finddir_fs(current, segment);
        if (!msg) return 0;
        current = msg;
    }
    
    return current;
}
