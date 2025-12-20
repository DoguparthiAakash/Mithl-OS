#ifndef VFS_H
#define VFS_H

#include "types.h"
#include <stddef.h>

#define FS_FILE        0x01
#define FS_DIRECTORY   0x02
#define FS_CHARDEVICE  0x03
#define FS_BLOCKDEVICE 0x04
#define FS_PIPE        0x05
#define FS_SYMLINK     0x06
#define FS_MOUNTPOINT  0x08

struct fs_node;

typedef uint32_t (*read_type_t)(struct fs_node*, uint32_t, uint32_t, uint8_t*);
typedef uint32_t (*write_type_t)(struct fs_node*, uint32_t, uint32_t, uint8_t*);
typedef void (*open_type_t)(struct fs_node*);
typedef void (*close_type_t)(struct fs_node*);
typedef struct dirent * (*readdir_type_t)(struct fs_node*, uint32_t);
typedef struct fs_node * (*finddir_type_t)(struct fs_node*, char *name);
typedef void (*create_type_t)(struct fs_node*, char *name, uint16_t permission);
typedef void (*mkdir_type_t)(struct fs_node*, char *name, uint16_t permission);

typedef struct fs_node {
    char name[128];
    uint32_t mask;        // Permissions mask
    uint32_t uid;         // Owning user
    uint32_t gid;         // Owning group
    uint32_t flags;       // Type of file (FS_FILE, etc.)
    uint32_t inode;       // Device-specific inode number
    uint32_t length;      // Size of the file in bytes
    uint32_t impl;        // Implementation specific number (e.g. sector)
    
    read_type_t read;
    write_type_t write;
    open_type_t open;
    close_type_t close;
    readdir_type_t readdir;
    finddir_type_t finddir;
    create_type_t create;
    mkdir_type_t mkdir;
    
    struct fs_node *ptr; // Used by mountpoints and symlinks
} fs_node_t;

struct dirent {
    char name[128];
    uint32_t ino;
};

// Global Root
extern fs_node_t *fs_root;

// Standard VFS calls
// Standard VFS calls
uint32_t read_fs(fs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer);
uint32_t write_fs(fs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer);
void open_fs(fs_node_t *node, uint8_t read, uint8_t write);
void close_fs(fs_node_t *node);
struct dirent *readdir_fs(fs_node_t *node, uint32_t index);
fs_node_t *finddir_fs(fs_node_t *node, char *name);
void create_fs(fs_node_t *parent, char *name, uint16_t permission);
void mkdir_fs(fs_node_t *parent, char *name, uint16_t permission);
void create_fs(fs_node_t *parent, char *name, uint16_t permission);
void mkdir_fs(fs_node_t *parent, char *name, uint16_t permission);

// Initialization
void vfs_init(void);

// Path Resolution Helper (Added for Terminal)
fs_node_t *vfs_resolve_path(const char *path);

#endif
