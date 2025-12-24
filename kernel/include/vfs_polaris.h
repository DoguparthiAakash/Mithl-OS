#ifndef VFS_POLARIS_H
#define VFS_POLARIS_H

#include "hashmap.h"
#include "resource_polaris.h"
#include "spinlock.h"
#include <stdbool.h>

// Renamed structs to avoid collision with legacy Mithl VFS
// struct vfs_node -> struct vfs_node_polaris

extern lock_t vfs_lock;

struct vfs_filesystem_polaris;

struct vfs_node_polaris {
	struct vfs_node_polaris *mountpoint;
	struct vfs_node_polaris *redir;
	struct resource *resource;
	struct vfs_filesystem_polaris *filesystem;
	char *name;
	struct vfs_node_polaris *parent;
	HASHMAP_TYPE(struct vfs_node_polaris *) children;
	char *symlink_target;
	bool populated;
};

typedef struct vfs_node_polaris *(*fs_mount_polaris_t)(struct vfs_node_polaris *, const char *,
									   struct vfs_node_polaris *);

struct vfs_filesystem_polaris {
	void (*populate)(struct vfs_filesystem_polaris *, struct vfs_node_polaris *);
	struct vfs_node_polaris *(*create)(struct vfs_filesystem_polaris *, struct vfs_node_polaris *,
							   const char *, int);
	struct vfs_node_polaris *(*symlink)(struct vfs_filesystem_polaris *, struct vfs_node_polaris *,
								const char *, const char *);
	struct vfs_node_polaris *(*link)(struct vfs_filesystem_polaris *, struct vfs_node_polaris *,
							 const char *, struct vfs_node_polaris *);
};

extern struct vfs_node_polaris *vfs_root_polaris;

void vfs_polaris_init(void);

struct vfs_node_polaris *vfs_create_node_polaris(struct vfs_filesystem_polaris *fs,
								 struct vfs_node_polaris *parent, const char *name,
								 bool dir);

void vfs_create_dotentries_polaris(struct vfs_node_polaris *node, struct vfs_node_polaris *parent);

// Path resolution helper (from vfs.c usually)
// struct path2node_res ...

#endif
