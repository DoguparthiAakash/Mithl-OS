#include <wm.h>
#include <gui.h>
#include <memory.h>
#include <string.h>
#include <console.h>

/* 
 * Mithl OS Window Manager
 * Inspired by Sway/i3
 * 
 * Located in windowmanager/ to serve as the core WM logic.
 */

extern void console_log(const char *msg);

static gui_window_t *wm_root = NULL;

#include <ports.h>
extern void serial_write(const char *msg);

void wm_init(void) {
    serial_write("[WM] Init Start...\n");
    // Create Root Container (Floating usually for Desktop)
    wm_root = (gui_window_t*)memory_alloc(sizeof(gui_window_t));
    if (!wm_root) { serial_write("[WM] Alloc Failed!\n"); return; }
    
    memset(wm_root, 0, sizeof(gui_window_t));
    
    // Root is a Container
    wm_root->is_container = 1;
    wm_root->layout_mode = LAYOUT_FLOATING; 
    wm_root->base.type = GUI_ELEMENT_WINDOW; // Treat as window/container
    
    // Initial bounds = Screen
    wm_root->base.bounds = (rect_t){0, 0, gui_mgr.screen_width, gui_mgr.screen_height};
    
    console_log("[WM] Sway-like Tree Initialized.\n");
    serial_write("[WM] Init Done.\n");
}

/* Recursive Render / Layout Engine */
void wm_render_tree(gui_window_t *node, rect_t bounds) {
    if (!node) return;
    
    rect_t content_bounds = bounds;
    
    // Calculate Children Layout
    if (node->is_container) {
        int count = 0;
        gui_window_t *child = node->wm_children;
        while(child) { count++; child = child->wm_next; }
        
        if (count == 0) return;
        
        child = node->wm_children;
        int i = 0;
        
        while (child) {
            rect_t child_bounds = content_bounds;
            
            if (node->layout_mode == LAYOUT_SPLIT_H) {
                // Horizontal Split (Equal for now)
                int w = content_bounds.width / count;
                child_bounds.x = content_bounds.x + i * w;
                child_bounds.width = w;
            }
            else if (node->layout_mode == LAYOUT_SPLIT_V) {
                // Vertical Split
                int h = content_bounds.height / count;
                child_bounds.y = content_bounds.y + i * h;
                child_bounds.height = h;
            }
            else if (node->layout_mode == LAYOUT_TABBED) {
                 // Tabbed logic placeholder
            }
            // FLOATING: Children keep their generic bounds (controlled by mouse/OS)
            
            if (node->layout_mode != LAYOUT_FLOATING) {
                // Enforce calculated bounds on child
                child->base.bounds = child_bounds;
            }
            
            // Recurse
            wm_render_tree(child, child->base.bounds);
            
            child = child->wm_next;
            i++;
        }
    }
}

void wm_manage_window(gui_window_t *window) {
    serial_write("[WM] Managing Window...\n");
    if (!wm_root) wm_init();
    
    // 1. Add to Tree (Generic Floating for now)
    window->wm_parent = wm_root;
    window->wm_next = wm_root->wm_children;
    wm_root->wm_children = window;
    
    // 2. Set Focus
    if (gui_mgr.root) {
        // Bring to front VISUALLY
        gui_bring_to_front((gui_element_t*)window);
        
        // Set Input Focus
        gui_mgr.focused_element = (gui_element_t*)window;
    }
    
    // 3. Trigger Layout Update
    // Exclude "Start" menu from layout recalculations (Floating Popup)
    if (window->title && strcmp(window->title, "Start") == 0) {
        // Do not trigger tree render for popup
    } else {
        wm_render_tree(wm_root, wm_root->base.bounds);
    }
    
    console_log("[WM] Managed Window: ");
    if (window->title) console_log(window->title);
    console_log("\n");
}

/* Create a new container for split/tabbed layouts */
gui_window_t *wm_create_container(layout_mode_t mode) {
    gui_window_t *container = (gui_window_t*)memory_alloc(sizeof(gui_window_t));
    if (!container) return NULL;
    
    memset(container, 0, sizeof(gui_window_t));
    container->is_container = 1;
    container->layout_mode = mode;
    container->split_ratio = 0.5f;
    container->base.type = GUI_ELEMENT_WINDOW;
    
    return container;
}

/* Add a child window/container to a parent container */
void wm_add_child(gui_window_t *parent, gui_window_t *child) {
    if (!parent || !child) return;
    
    child->wm_parent = parent;
    child->wm_next = parent->wm_children;
    parent->wm_children = child;
    
    // Recalculate layout
    if (wm_root) {
        wm_render_tree(wm_root, wm_root->base.bounds);
    }
}

/* Handle events through the WM tree (placeholder for future enhancements) */
void wm_handle_event(gui_window_t *node, gui_event_t *event) {
    if (!node || !event) return;
    
    // For now, just pass through to standard event handler
    // Future: Add WM-specific event handling (tiling shortcuts, etc.)
    if (node->base.event_handler) {
        node->base.event_handler((gui_element_t*)node, event);
    }
}
