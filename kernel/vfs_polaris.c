#include "vfs_polaris.h"
#include "hashmap.h"
#include "spinlock.h"
#include "memory.h"
#include "string.h"
#include "console.h"

// Parallel VFS Port (Polaris Logic)
// Adapted for Mithl-OS

#define PATH_MAX 4096

// Error numbers (Simplified shim)
#define ENOENT 2
#define ENOTDIR 20

static int errno = 0; // Local errno for this module

lock_t vfs_lock = {0, 0}; // Init lock

// Helper: strdup shim
static char *strdup(const char *s) {
    if (!s) return NULL;
    int len = strlen(s);
    char *new = memory_alloc(len + 1);
    if (new) strcpy(new, s);
    return new;
}

struct vfs_node_polaris *vfs_create_node_polaris(struct vfs_filesystem_polaris *fs,
								 struct vfs_node_polaris *parent, const char *name,
								 bool dir) {
	struct vfs_node_polaris *node = memory_alloc(sizeof(struct vfs_node_polaris));
    if (!node) return NULL;
    
    // Zero out
    memset(node, 0, sizeof(struct vfs_node_polaris));

	node->name = strdup(name);
	node->parent = parent;
	node->filesystem = fs;

	if (dir) {
		// Initialize Hashmap (macro usage adapted)
        // HASHMAP_INIT returns a compound literal which struct copy handles?
        // Or we need to init the struct fields manually if HASHMAP_INIT relies on specific syntax.
        // HASHMAP_INIT(CAP) {.cap = (CAP), .buckets = NULL}
		node->children = (typeof(node->children))HASHMAP_INIT(256);
	}

	return node;
}

void vfs_create_dotentries_polaris(struct vfs_node_polaris *node, struct vfs_node_polaris *parent) {
	struct vfs_node_polaris *dot = vfs_create_node_polaris(node->filesystem, node, ".", false);
	struct vfs_node_polaris *dotdot = vfs_create_node_polaris(node->filesystem, node, "..", false);

	dot->redir = node;
	dotdot->redir = parent;

	HASHMAP_SINSERT(&node->children, ".", dot);
	HASHMAP_SINSERT(&node->children, "..", dotdot);
}

// Global Filesystem Map
static HASHMAP_TYPE(fs_mount_polaris_t) filesystems;

struct vfs_node_polaris *vfs_root_polaris = NULL;

void vfs_polaris_init(void) {
	console_write("[VFS-Polaris] Initializing...\n");
    vfs_root_polaris = vfs_create_node_polaris(NULL, NULL, "", false);
	// filesystems = (typeof(filesystems))HASHMAP_INIT(256);
    // Standard C trick for hashmap init if macro fails in function body for some compilers:
    filesystems.cap = 256;
    filesystems.buckets = NULL;

    console_write("[VFS-Polaris] Root Node Created.\n");
}

// Minimal path resolution shim (to verify compilation)
// Fully implementing path2node requires `reduce_node` and `populate`, which depend on `resource` methods.
// For the initial port, we stub them or implement basic traversal.

/*
static struct vfs_node_polaris *reduce_node(struct vfs_node_polaris *node, bool follow_symlinks) {
    // Stub
    return node; 
}
*/

// Stub to satisfy linker if needed
void vfs_add_filesystem(void *fs, const char *id) {
    (void)fs; (void)id;
}
