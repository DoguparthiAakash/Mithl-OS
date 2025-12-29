#include "vfs.h"
#include "string.h"
#include "console.h"
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

// === VFS 2.0 Implementation ===

static mount_point_t *mount_list = NULL;
static fs_driver_t *driver_list = NULL;

void vfs_register_driver(const char *name, mount_callback mount) {
    fs_driver_t *drv = (fs_driver_t*)memory_alloc(sizeof(fs_driver_t));
    strcpy(drv->name, name);
    drv->mount = mount;
    drv->next = driver_list;
    driver_list = drv;
    console_write("[VFS] Registered filesystem: ");
    console_write(name);
    console_write("\n");
}

int vfs_mount(const char *source, const char *target, const char *fs_type) {
    // 1. Find Driver
    fs_driver_t *drv = driver_list;
    while(drv) {
        if (strcmp(drv->name, fs_type) == 0) break;
        drv = drv->next;
    }
    if (!drv) {
        console_write("[VFS] Error: Filesystem driver not found: ");
        console_write(fs_type);
        console_write("\n");
        return -1;
    }

    // 2. Resolve Target (Must exist and be a directory)
    // Special case: "/" (Root mount) happens before VFS is fully active?
    // If target is NULL or "/" and fs_root is NULL, we are mounting root.
    if ((!target || strcmp(target, "/") == 0) && !fs_root) {
        fs_root = drv->mount(source, NULL);
        console_write("[VFS] Mounted Root\n");
        return 0;
    }

    fs_node_t *mountpoint_node = vfs_resolve_path(target);
    if (!mountpoint_node) return -1;
    if ((mountpoint_node->flags & 0x7) != FS_DIRECTORY) return -1;

    // 3. Mount
    fs_node_t *root = drv->mount(source, target);
    if (!root) return -1;

    // 4. Create Mount Record
    mount_point_t *mp = (mount_point_t*)memory_alloc(sizeof(mount_point_t));
    strcpy(mp->path, target);
    mp->root = root;
    mp->next = mount_list;
    mount_list = mp;
    
    // Mark node as mountpoint so we cross it
    mountpoint_node->flags |= FS_MOUNTPOINT;
    mountpoint_node->ptr = root; // Link to new root

    console_write("[VFS] Mounted ");
    console_write(fs_type);
    console_write(" at ");
    console_write(target);
    console_write("\n");
    
    return 0;
}

// Updated Path Resolver to handle Mount Points
fs_node_t *vfs_resolve_path(const char *path) {
    if (!path) return NULL;
    // Handle Root
    fs_node_t *current = fs_root;
    if (!current) return NULL;
    
    char path_buf[256];
    strcpy(path_buf, path);
    
    // Normalize absolute path
    char *p = path_buf;
    if (*p == '/') {
        current = fs_root;
        while(*p == '/') p++; // Skip leading slashes
    } else {
        // Relative path not really supported without CWD context passed in
        // Assume root for now
    }
    
    char *segment = p;
    while (*p) {
        if (*p == '/') {
            *p = 0; // Terminate segment
            
            if (strlen(segment) > 0) {
                // before descending, check if current is a mountpoint (though normally 'ptr' is used implicitly?)
                // If we are AT a mountpoint, 'current' is the underlying node.
                // But we should have swapped 'current' with the mounted root already if we traversed.
                
                fs_node_t *next = finddir_fs(current, segment);
                
                // Mount Point Traversal (Down)
                if (next && (next->flags & FS_MOUNTPOINT)) {
                    // Switch to the mounted root
                    if (next->ptr) next = next->ptr;
                }
                
                if (!next) return NULL;
                current = next;
            }
            segment = p + 1;
        }
        p++;
    }
    
    // Last segment
    if (strlen(segment) > 0) {
        fs_node_t *next = finddir_fs(current, segment);
         if (next && (next->flags & FS_MOUNTPOINT)) {
            if (next->ptr) next = next->ptr;
        }
        if (!next) return NULL;
        current = next;
    }
    
    return current;
}

// Stubs for Linker Satisfaction

int vfs_move(const char *src, const char *dest) {
    (void)src; (void)dest;
    console_write("[VFS] Warning: vfs_move not implemented.\n");
    return -1;
}

int vfs_rename(fs_node_t *parent, const char *oldpath, const char *newpath) {
    (void)parent; (void)oldpath; (void)newpath;
    console_write("[VFS] Warning: vfs_rename not implemented.\n");
    return -1;
}

int vfs_copy(const char *src, const char *dest) {
     (void)src; (void)dest;
     console_write("[VFS] Warning: vfs_copy not implemented.\n");
     return -1;
}

int vfs_delete(fs_node_t *parent, const char *path) {
     (void)parent; (void)path;
     console_write("[VFS] Warning: vfs_delete partial stub.\n");
     return -1;
}


