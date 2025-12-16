#ifndef WM_H
#define WM_H

#include "gui.h"

// Initialize the Window Manager Tree
void wm_init(void);

// Create a new container (split/tab) or window wrapper
gui_window_t *wm_create_container(layout_mode_t mode);

// Add a window/container to the tree
void wm_add_child(gui_window_t *parent, gui_window_t *child);

// Recursive Layout Calculation (i3-like)
void wm_render_tree(gui_window_t *node, rect_t bounds);

// Handle Events through the tree
void wm_handle_event(gui_window_t *node, gui_event_t *event);

// Helper to wrap existing windows into the tree
void wm_manage_window(gui_window_t *window);

#endif
