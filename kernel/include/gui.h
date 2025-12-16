#ifndef GUI_H
#define GUI_H

#include "graphics.h"
#include "list.h" // Assuming a generic linked list implementation exists

/* Forward declarations */
struct gui_element;
struct gui_manager;

/* Tab structure */
typedef struct gui_tab
{
    char *title;
    struct gui_element *content; // The root element for this tab's content
    struct gui_tab *next;
} gui_tab_t;

/* GUI Element types */
typedef enum
{
    GUI_ELEMENT_WINDOW,
    GUI_ELEMENT_BUTTON,
    GUI_ELEMENT_LABEL,
    GUI_ELEMENT_PANEL
} gui_element_type_t;

/* GUI Element states */
typedef enum
{
    GUI_STATE_NORMAL,
    GUI_STATE_HOVER,
    GUI_STATE_PRESSED,
    GUI_STATE_DISABLED
} gui_state_t;

/* gui_renderer_t is in types.h */

/* Event types */
typedef enum
{
    GUI_EVENT_MOUSE_DOWN,
    GUI_EVENT_MOUSE_UP,
    GUI_EVENT_MOUSE_MOVE,
    GUI_EVENT_MOUSE_SCROLL,
    GUI_EVENT_KEY_PRESS
} gui_event_type_t;

/* --- Layout Management (i3-inspired) --- */
typedef enum {
    LAYOUT_NONE,        // Simple element (button, label)
    LAYOUT_FLOATING,    // Standard macOS behavior (overlays)
    LAYOUT_SPLIT_H,     // Horizontal Tiling
    LAYOUT_SPLIT_V,     // Vertical Tiling
    LAYOUT_TABBED,      // macOS/i3 Tabbed container
    LAYOUT_STACKED      // i3 Stacked (rarely used in macOS, but good to have)
} layout_mode_t;

/* Forward declaration for Window Manager Tree Node */
// Avoid incomplete type issues by careful ordering or casting.
// gui_window_t is defined below. 
// We use 'struct gui_window' in the struct definition inside gui.h?
// The struct definition uses 'struct gui_window *wm_parent'.
// This is fine if 'struct gui_window' matches typedef ???
// C allows: typedef struct tag { ... } name;
// Let's check how gui_window_t is defined later.
// It is anonymous struct? "typedef struct { ... } gui_window_t;"
// THIS IS THE ISSUE. Anonymous struct cannot be referenced by 'struct gui_window'.
// Should change definition to "typedef struct gui_window { ... } gui_window_t;"
 

/* Event structure */
typedef struct
{
    gui_event_type_t type;
    union
    {
        struct
        {
            point_t pos;
            int button; // 1=left, 2=right, 4=middle
            int scroll_delta; // +1 or -1 usually
        } mouse;
        struct
        {
            char key;
        } keyboard;
    };
} gui_event_t;


/* Base GUI Element structure */
typedef struct gui_element
{
    gui_element_type_t type;
    gui_state_t state;
    rect_t bounds;
    uint32_t background_color;
    uint32_t border_color;
    uint32_t text_color;
    char *text;
    struct gui_element *parent;
    list_t *children; // Use a linked list for children
    void (*event_handler)(struct gui_element *element, gui_event_t *event);
    void (*draw)(gui_renderer_t *renderer, struct gui_element *element);
} gui_element_t;

/* Window structure */
typedef struct gui_window
{
    gui_element_t base;
    char *title;
    int is_dragging;
    point_t drag_offset;
    // Window Management State
    rect_t saved_bounds;
    int is_maximized;
    int is_minimized;
    
    // Tab Support
    list_t *tabs;          // List of gui_tab_t
    gui_tab_t *active_tab; // Currently active tab
    
    /* --- WM Tree Properties --- */
    struct gui_window *wm_parent;    // Parent Container in WM Tree
    struct gui_window *wm_children;  // Head of children list (if container)
    struct gui_window *wm_next;      // Next sibling
    
    layout_mode_t layout_mode;       // How children are arranged
    float split_ratio;               // 0.5 default. Percentage of parent space.
    
    int is_container;                // 1 if this is a split node, 0 if leaf content
} gui_window_t;

/* Button structure */
typedef struct
{
    gui_element_t base;
} gui_button_t;

/* Label structure */
typedef struct
{
    gui_element_t base;
    // Removed specific styling; theme handles this
} gui_label_t;

/* Panel structure */
typedef struct
{
    gui_element_t base;
    int has_border;
    int border_width;
    // Removed specific styling; theme handles this
} gui_panel_t;

/* GUI Manager structure */
typedef struct gui_manager
{
    gui_element_t *root;
    gui_element_t *focused_element;
    gui_element_t *hovered_element;
    gui_renderer_t renderer;
    list_t *event_queue; // Event queue for processing
    int screen_width;
    int screen_height;
    // Dirty Rectangle Optimization
    int needs_redraw;
    rect_t dirty_rect; // Union of dirty areas
} gui_manager_t;

/* Global GUI manager instance */
extern gui_manager_t gui_mgr;

/* Function prototypes */
void gui_init(int screen_width, int screen_height, gui_renderer_t *renderer);
void gui_run(void); // Main loop for GUI
void gui_add_element(gui_element_t *parent, gui_element_t *child);
void gui_remove_element(gui_element_t *element);
void gui_free_element(gui_element_t *element);
void gui_post_event(gui_event_t *event);
void gui_set_position(gui_element_t *element, int x, int y);
void gui_invalidate_rect(rect_t rect); // Mark region as dirty
void gui_present(void); // Present backbuffer to screen
void gui_present(void); // Present backbuffer to screen
gui_element_t *gui_find_element_at(int x, int y);
void gui_bring_to_front(gui_element_t *element);
void gui_window_add_tab(gui_window_t *window, const char *title, gui_element_t *content);

int point_in_rect(point_t p, rect_t r);

/* Element creation functions */
gui_window_t *gui_create_window(const char *title, int x, int y, int width, int height);
gui_button_t *gui_create_button(const char *text, int x, int y, int width, int height);
gui_label_t *gui_create_label(const char *text, int x, int y, int width, int height);
gui_panel_t *gui_create_panel(int x, int y, int width, int height);

/* Drawing functions (can be generic and theme-dependent) */
void gui_draw_element_default(gui_renderer_t *renderer, gui_element_t *element);
void gui_draw_recursive(gui_renderer_t *renderer, gui_element_t *element);
void gui_draw_window(gui_renderer_t *renderer, gui_element_t *element);
void gui_draw_window_macos(gui_renderer_t *renderer, gui_element_t *element);

/* Event Handlers */
void gui_default_event_handler(gui_element_t *element, gui_event_t *event);
void gui_window_event_handler(gui_element_t *element, gui_event_t *event);
void gui_draw_button_macos(gui_renderer_t *renderer, gui_element_t *element);
// etc.

/* Default handler */
void gui_default_event_handler(gui_element_t *element, gui_event_t *event);

#endif /* GUI_H */