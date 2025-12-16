#include "wm.h"
#include "gui.h"
#include "memory.h"
#include "string.h"
#include "string.h"
#include "console.h"

extern void console_log(const char *msg);

static gui_window_t *wm_root = NULL;

void wm_init(void) {
    // Create Root Container (Floating usually for Desktop)
    wm_root = (gui_window_t*)memory_alloc(sizeof(gui_window_t));
    memset(wm_root, 0, sizeof(gui_window_t));
    
    // Root is a Container
    wm_root->is_container = 1;
    wm_root->layout_mode = LAYOUT_FLOATING; 
    wm_root->base.type = GUI_ELEMENT_WINDOW; // Treat as window/container
    
    // Initial bounds = Screen
    wm_root->base.bounds = (rect_t){0, 0, gui_mgr.screen_width, gui_mgr.screen_height};
    
    console_log("[WM] Tree Initialized.\n");
}

/* Recursive Render / Layout Engine */
void wm_render_tree(gui_window_t *node, rect_t bounds) {
    if (!node) return;
    
    // Update Node Bounds
    // For FLOATING, we typically respect the window's own bounds unless it's root
    // For SPLIT/TABBED, we enforce bounds.
    
    // 1. Draw Self (if it has content/deco)
    // Containers might draw a background or border
    // Windows draw themselves via standard gui draw?
    // Actually, this function is for LAYOUT specific logic.
    // Standard drawing happens via gui_draw_recursive?
    // We should probably separate Layout Calc from Drawing.
    
    // Let's assume this function JUST CALCULATES LAYOUT.
    
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
                 // Tabbed: Full size, but only top visible?
                 // Or draw all with tabs on top.
                 // For bounds, they get full content area.
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
    if (!wm_root) wm_init();
    
    // 1. Add to Tree (Generic Floating for now)
    // In i3, this would be: con_attach(new_con, focused, false);
    
    // We attach to the ROOT's children for now.
    window->wm_parent = wm_root;
    window->wm_next = wm_root->wm_children;
    wm_root->wm_children = window;
    
    // 2. Set Focus (i3: con_focus(con))
    // We need to implement a basic "active window" concept in WM or GUI.
    // GUI manager has `focused_element`. WM should sync with it.
    if (gui_mgr.root) {
        // Bring to front VISUALLY
        gui_bring_to_front((gui_element_t*)window);
        
        // Set Input Focus
        gui_mgr.focused_element = (gui_element_t*)window;
    }
    
    // 3. Trigger Layout Update (i3: tree_render)
    // Since we added a child, we must re-calculate bounds if tiling.
    // If floating, we just ensure it's on screen.
    
    wm_render_tree(wm_root, wm_root->base.bounds);
    
    // 4. Set Mapped State (Visible)
    // Window starts visible? Yes.
    
    console_log("[WM] Managed Window: ");
    if (window->title) console_log(window->title);
    console_log("\n");
}
