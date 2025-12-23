#ifndef SYNC_EVENT_H
#define SYNC_EVENT_H

#include "spinlock.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "types.h"

// Renamed from 'event' to 'sync_event' to avoid conflict with GUI 'event_t'
#define EVENT_MAX_LISTENERS 32

struct thread; // Forward decl

struct sync_event_listener {
	struct thread *thread;
	size_t which;
};

struct sync_event {
	lock_t lock;
	size_t pending;
	size_t listeners_i;
	struct sync_event_listener listeners[EVENT_MAX_LISTENERS];
};

// Implementations will be in vfs_polaris.c or separate sync.c
// For now, headers only.
// ssize_t sync_event_await(struct sync_event **events, size_t num_events, bool block);
// size_t sync_event_trigger(struct sync_event *event, bool drop);

#endif
