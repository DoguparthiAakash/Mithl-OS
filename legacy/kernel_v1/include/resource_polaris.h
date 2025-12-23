#ifndef RESOURCE_POLARIS_H
#define RESOURCE_POLARIS_H

#include "sync_event.h"
#include "spinlock.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "types.h"
// #include <sched/syscall.h> // Mithl syscalls are different

// Forward decls
struct process;
struct f_description;
struct vfs_node_polaris; // Renamed to avoid collision with vfs.h vfs_node
struct stat; // Standard struct, check if defined in types.h or similar

// Missing Types Shim
typedef uint32_t mode_t;
typedef uint32_t uid_t;
typedef uint32_t gid_t;
typedef int32_t off_t;
typedef int32_t ssize_t;
// typedef int32_t pid_t; // Might need this too

// From Polaris klibc/resource.h
struct resource {
	int status;
	struct sync_event event; // Renamed
	size_t refcount;
	lock_t lock;
	// struct stat stat; // Defined in types.h usually? Or sys/stat.h
    // Mithl types.h has no stat. Let's define it or include a shim.
    // For now, simple placeholder or simplified stat.
    struct {
        mode_t st_mode;
        uid_t st_uid;
        gid_t st_gid;
        off_t st_size;
        // ... add others as needed
    } stat;

	bool can_mmap;

	ssize_t (*read)(struct resource *this, struct f_description *description,
					void *buf, off_t offset, size_t count);
	ssize_t (*write)(struct resource *this, struct f_description *description,
					 const void *buf, off_t offset, size_t count);
	int (*ioctl)(struct resource *this, struct f_description *description,
				 uint64_t request, uint64_t arg);
	void *(*mmap)(struct resource *this, size_t file_page, int flags);
	bool (*ref)(struct resource *this, struct f_description *description);
	bool (*unref)(struct resource *this, struct f_description *description);
	bool (*truncate)(struct resource *this, struct f_description *description,
					 size_t length);
};

struct f_description {
	size_t refcount;
	off_t offset;
	bool is_dir;
	int flags;
	lock_t lock;
	struct resource *res;
	struct vfs_node_polaris *node;
};

struct f_descriptor {
	struct f_description *description;
	int flags;
};

// Flags (Standard POSIX-ish)
#define O_RDONLY    0
#define O_WRONLY    1
#define O_RDWR      2
#define O_CREAT     64
#define O_EXCL      128
#define O_NOCTTY    256
#define O_TRUNC     512
#define O_APPEND    1024
#define O_NONBLOCK  2048
#define O_DIRECTORY 65536
#define O_CLOEXEC   524288

// File Creation Masks
#define S_IFMT  0170000
#define S_IFSOCK 0140000
#define S_IFLNK 0120000
#define S_IFREG 0100000
#define S_IFBLK 0060000
#define S_IFDIR 0040000
#define S_IFCHR 0020000
#define S_IFIFO 0010000

#define S_ISLNK(m) (((m) & S_IFMT) == S_IFLNK)
#define S_ISREG(m) (((m) & S_IFMT) == S_IFREG)
#define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)
#define S_ISCHR(m) (((m) & S_IFMT) == S_IFCHR)
#define S_ISBLK(m) (((m) & S_IFMT) == S_IFBLK)
#define S_ISFIFO(m) (((m) & S_IFMT) == S_IFIFO)
#define S_ISSOCK(m) (((m) & S_IFMT) == S_IFSOCK)

void *resource_create(size_t size);

#endif
