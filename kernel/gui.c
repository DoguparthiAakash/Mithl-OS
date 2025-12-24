#include "gui.h"
#include "memory.h"
#include "list.h"
#include "vga.h"
#include "string.h"
#include "wm.h"
#include <theme.h>
#include "process.h" // For current_process

extern process_t *current_process;

gui_manager_t gui_mgr;

// Helper function to check if a point is within a rectangle
int point_in_rect(point_t p, rect_t r)
{
    return (p.x >= r.x && p.x < r.x + r.width &&
            p.y >= r.y && p.y < r.y + r.height);
}

// Helper function to check if two rectangles intersect
static int rect_intersects(rect_t r1, rect_t r2) {
    return !(r1.x + r1.width <= r2.x || 
             r2.x + r2.width <= r1.x ||
             r1.y + r1.height <= r2.y ||
             r2.y + r2.height <= r1.y);
}

// Recursive helper to find top-most element
static gui_element_t *gui_find_recursive(gui_element_t *root, int x, int y) {
    if (!root) return NULL;
    
    point_t p = {x, y};
    // Check if point is inside this element
    if (!point_in_rect(p, root->bounds)) {
        return NULL;
    }
    
    // It is inside. Now checking children.
    // We want the last child that contains the point (top-most z-order).
    gui_element_t *found = root; // Default to self
    
    if (root->children) {
        list_node_t *node = root->children->head;
        while (node) {
            gui_element_t *child = (gui_element_t*)node->data;
            gui_element_t *result = gui_find_recursive(child, x, y);
            if (result) {
                found = result; // Found a better match (higher z-order)
            }
            node = node->next;
        }
    }
    
    return found;
}

// Function to find the top-most element at a given position.
gui_element_t *gui_find_element_at(int x, int y)
{
    if (!gui_mgr.root) return NULL;
    return gui_find_recursive(gui_mgr.root, x, y);
}

// Default event handler for all elements
void gui_default_event_handler(gui_element_t *element, gui_event_t *event)
{
    if (!element || !event)
        return;

    switch (event->type)
    {
    case GUI_EVENT_MOUSE_DOWN:
        if (point_in_rect(event->mouse.pos, element->bounds))
        {
            element->state = GUI_STATE_PRESSED;
            gui_mgr.focused_element = element;
        }
        break;
    case GUI_EVENT_MOUSE_UP:
        if (element->state == GUI_STATE_PRESSED)
        {
            element->state = GUI_STATE_NORMAL;
            if (element == gui_mgr.focused_element)
            {
                // Trigger click event if needed
            }
        }
        break;
    case GUI_EVENT_MOUSE_MOVE:
        if (point_in_rect(event->mouse.pos, element->bounds))
        {
            if (element->state == GUI_STATE_NORMAL)
            {
                element->state = GUI_STATE_HOVER;
            }
        }
        else if (element->state == GUI_STATE_HOVER)
        {
            element->state = GUI_STATE_NORMAL;
            gui_mgr.hovered_element = NULL; // Clear hovered state
        }
        break;
    default:
        break;
    }
}

// Forward declaration
void gui_draw_recursive(gui_renderer_t *renderer, gui_element_t *element);

// Default drawing function (can be overridden by themes)
void gui_draw_element_default(gui_renderer_t *renderer, gui_element_t *element)
{
    if (!element || !renderer)
        return;

    // Draw background
    renderer->draw_rect(element->bounds, element->background_color);

    // Draw border (simple example)
    // if (element->border_color) { ... }

    // Draw text (simple example)
    if (element->text)
    {
        point_t pos = {element->bounds.x, element->bounds.y};
        renderer->draw_text(element->text, pos, element->text_color);
    }
}

// Draw the entire GUI tree starting from the root
void gui_draw(void)
{
    if (gui_mgr.root)
    {
        // vga_clear_buffer(); // Disable legacy VGA clear if we are in VESA mode, or let it be if it's safe.
        // For now, let's just call recursive draw.
        // If vga_clear_buffer() does text mode stuff, it might be harmless or glitchy on VESA.
        // But since we want to fix build, I will just keep the logic simple.
        gui_draw_recursive(&gui_mgr.renderer, gui_mgr.root);
    }
}
// Recursive draw function to render all elements and their children
void gui_draw_recursive(gui_renderer_t *renderer, gui_element_t *element)
{
    if (!element)
        return;

    // OPTIMIZATION: Skip elements outside dirty region
    if (gui_mgr.needs_redraw && gui_mgr.dirty_rect.width > 0 && gui_mgr.dirty_rect.height > 0) {
        if (!rect_intersects(element->bounds, gui_mgr.dirty_rect)) {
            // Element is completely outside dirty region, skip it and all children
            return;
        }
    }

    // Draw the element itself
    if (element->draw)
    {
        element->draw(renderer, element);
    }
    else
    {
        gui_draw_element_default(renderer, element);
    }

    // Draw children
    if (element->children)
    {
        list_node_t *node = element->children->head;
        while (node)
        {
            gui_draw_recursive(renderer, (gui_element_t *)node->data);
            node = node->next;
        }
    }
}

// Post an event to the queue
void gui_post_event(gui_event_t *event)
{
    extern void serial_write(const char*);
    serial_write("[GUI] Posting Event...\n");
    
    if (gui_mgr.event_queue && event)
    {
        serial_write("[GUI] Queue valid, appending...\n");
        list_append(gui_mgr.event_queue, event);
        serial_write("[GUI] Event Posted.\n");
    } else {
        serial_write("[GUI] CRITICAL: Event Queue is NULL!\n");
    }
}

// Extern for global shortcuts
extern void toggle_start_menu(void);

// Helper to union two rects (bounding box)
static rect_t util_rect_union(rect_t r1, rect_t r2) {
    if (r1.width == 0 && r1.height == 0) return r2;
    if (r2.width == 0 && r2.height == 0) return r1;
    
    int x1 = (r1.x < r2.x) ? r1.x : r2.x;
    int y1 = (r1.y < r2.y) ? r1.y : r2.y;
    int x2_1 = r1.x + r1.width;
    int x2_2 = r2.x + r2.width;
    int x2 = (x2_1 > x2_2) ? x2_1 : x2_2;
    int y2_1 = r1.y + r1.height;
    int y2_2 = r2.y + r2.height;
    int y2 = (y2_1 > y2_2) ? y2_1 : y2_2;
    
    return (rect_t){x1, y1, x2 - x1, y2 - y1};
}

void gui_invalidate_rect(rect_t rect) {
    if (rect.width <= 0 || rect.height <= 0) return;
    
    // Union with existing dirty rect
    if (gui_mgr.needs_redraw == 0) {
        gui_mgr.dirty_rect = rect;
        gui_mgr.needs_redraw = 1;
    } else {
        gui_mgr.dirty_rect = util_rect_union(gui_mgr.dirty_rect, rect);
    }
    
    // Union with background dirty rect (Always redraw background for invalid areas)
    if (gui_mgr.needs_background_redraw == 0) {
        gui_mgr.background_dirty_rect = rect;
        gui_mgr.needs_background_redraw = 1;
    } else {
        gui_mgr.background_dirty_rect = util_rect_union(gui_mgr.background_dirty_rect, rect);
    }
}

// Mouse Capture Implementation
void gui_capture_mouse(gui_element_t *element) {
    gui_mgr.captured_element = element;
}

void gui_release_mouse(void) {
    gui_mgr.captured_element = NULL;
}

// Main GUI update/draw loop
void gui_run(void)
{
    // Process all events in the queue
    gui_event_t *event;
    while ((event = list_pop_front(gui_mgr.event_queue)))
    {
        // Global Shortcuts (intercept before dispatch)
        if (event->type == GUI_EVENT_KEY_PRESS) {
            uint8_t code = (uint8_t)event->keyboard.key;
            // Windows Key (0x5B Left, 0x5C Right)
            if (code == 0x5B || code == 0x5C) {
                toggle_start_menu();
                memory_free(event);
                // toggle_start_menu invalidates internally
                continue;
            }
        }

        gui_element_t *target = NULL;
        
        // Routing Logic
        if (gui_mgr.captured_element) {
            // Priority: Captured Element receives ALL mouse events
            if (event->type == GUI_EVENT_MOUSE_DOWN || 
                event->type == GUI_EVENT_MOUSE_UP || 
                event->type == GUI_EVENT_MOUSE_MOVE ||
                event->type == GUI_EVENT_MOUSE_SCROLL) {
                target = gui_mgr.captured_element;
            } else if (event->type == GUI_EVENT_KEY_PRESS) {
                 target = gui_mgr.focused_element;
            }
        }
        else if (event->type == GUI_EVENT_KEY_PRESS) {
            target = gui_mgr.focused_element;
            if (!target) target = gui_mgr.root; 
            if(target) gui_invalidate_rect(target->bounds); // Invalidate target to be safe
        } else {
            target = gui_find_element_at(event->mouse.pos.x, event->mouse.pos.y);
            
            if ((event->type == GUI_EVENT_MOUSE_DOWN) && target) {
                 if (gui_mgr.focused_element != target) {
                     if(gui_mgr.focused_element) gui_invalidate_rect(gui_mgr.focused_element->bounds); // Redraw old focus
                     gui_mgr.focused_element = target;
                     gui_invalidate_rect(target->bounds); // Redraw new focus
                 }
            }
        }

        if (target && target->event_handler)
        {
            // OPTIMIZATION: Track state changes to avoid unnecessary invalidations
            int prev_state = target->state;
            gui_element_t *prev_hovered = gui_mgr.hovered_element;
            
            target->event_handler(target, event);

            // Userspace Dispatch: If target is a window owned by a user process
            {
                 gui_window_t *win = NULL;
                 if (target->type == GUI_ELEMENT_WINDOW) win = (gui_window_t*)target;
                 // If target is child of window, find window?
                 // For now simplicity: Just check if 'target' itself is window (since we dispatch to window handler)
                 
                 // If we found a window and it has an owner
                 if (win && win->owner_pid > 0 && win->incoming_events) {
                      // Clone event
                      gui_event_t *copy = (gui_event_t*)memory_alloc(sizeof(gui_event_t));
                      if (copy) {
                          *copy = *event;
                          // Convert to Window Relative Coords if Mouse Event
                          if (event->type == GUI_EVENT_MOUSE_DOWN || 
                              event->type == GUI_EVENT_MOUSE_UP || 
                              event->type == GUI_EVENT_MOUSE_MOVE ||
                              event->type == GUI_EVENT_MOUSE_SCROLL) {
                               
                               int title_h = 30; // Consistent Height
                               copy->mouse.pos.x -= win->base.bounds.x;
                               copy->mouse.pos.y -= (win->base.bounds.y + title_h);
                          }
                          
                          // Critical Section: Protect list modification from Syscall preemption
                          asm volatile("cli");
                          list_append(win->incoming_events, copy);
                          asm volatile("sti");
                      }
                 }
            }
            
            // Only invalidate on actual state changes or non-root elements
            if (event->type == GUI_EVENT_MOUSE_MOVE) {
                // For mouse moves, only invalidate if state changed or hover changed
                if ((int)target->state != prev_state) {
                    gui_invalidate_rect(target->bounds);
                }
                // Update hovered element tracking
                if (target != gui_mgr.root) {
                    if (prev_hovered != target) {
                        if (prev_hovered && prev_hovered != gui_mgr.root) {
                            gui_invalidate_rect(prev_hovered->bounds);
                        }
                        gui_mgr.hovered_element = target;
                        gui_invalidate_rect(target->bounds);
                    }
                }
            } else {
                // For other events, invalidate if state changed or if non-root
                if ((int)target->state != prev_state || target != gui_mgr.root) {
                    gui_invalidate_rect(target->bounds);
                }
            }
        }
        memory_free(event);
    }

    // Draw all elements only if dirty
    if (gui_mgr.needs_redraw) {
        // LAYER-BASED RENDERING: Redraw background first if needed
        if (gui_mgr.needs_background_redraw) {
            // Set clip to background dirty rect
            graphics_set_clip(gui_mgr.background_dirty_rect);
            
            // Redraw desktop background (wallpaper + top bar + dock) for dirty region
            // Redraw desktop background (wallpaper + top bar + dock) for dirty region
            // Since external desktop_draw_rect source is missing/unreliable, we use a local fallback
            // that draws a solid color or pattern to ensure the buffer is cleared.
            draw_rect_filled(gui_mgr.background_dirty_rect, 0xFF336699); // Solid Slate Blue Background
            
            // If we had a wallpaper function, we'd call it here.
            // draw_wallpaper(gui_mgr.background_dirty_rect);
            
            // Reset background redraw state
            gui_mgr.needs_background_redraw = 0;
            gui_mgr.background_dirty_rect = (rect_t){0, 0, 0, 0};
        }
        
        // Set clip rect in graphics (draw to backbuffer)
        graphics_set_clip(gui_mgr.dirty_rect);
        
        gui_draw_recursive(&gui_mgr.renderer, gui_mgr.root);
        
        // Reset clip but DO NOT clear dirty state yet.
        // We need to present (swap) this dirty region to the screen first.
        graphics_set_clip((rect_t){0, 0, gui_mgr.screen_width, gui_mgr.screen_height});
    }
}

// Present the backbuffer to video memory (Only updates dirty region)
void gui_present(void) {
    if (gui_mgr.needs_redraw) {
        // Copy ONLY the dirty rectangle from backbuffer to VRAM
        graphics_copy_rect(gui_mgr.dirty_rect.x, gui_mgr.dirty_rect.y, 
                           gui_mgr.dirty_rect.width, gui_mgr.dirty_rect.height);
        
        // Reset state
        gui_mgr.needs_redraw = 0;
        gui_mgr.dirty_rect = (rect_t){0, 0, 0, 0};
    }
}

// gui_create_panel() - Implementation
gui_panel_t *gui_create_panel(int x, int y, int width, int height)
{
    gui_panel_t *panel = (gui_panel_t *)memory_alloc(sizeof(gui_panel_t));
    if (!panel)
        return NULL;

    // Initialize base gui_element_t members
    panel->base.type = GUI_ELEMENT_PANEL;
    panel->base.state = GUI_STATE_NORMAL;
    panel->base.bounds.x = x;
    panel->base.bounds.y = y;
    panel->base.bounds.width = width;
    panel->base.bounds.height = height;
    panel->base.text = NULL;
    panel->base.parent = NULL;
    panel->base.children = list_create(); // Initialize children list
    panel->base.event_handler = gui_default_event_handler;
    panel->base.draw = gui_draw_element_default;

    // Initialize gui_panel_t specific members
    panel->has_border = 0;
    panel->border_width = 0;
    panel->base.background_color = 0xFFCCCCCC; // Default color

    return panel;
}

// Initialize GUI system and manager
void gui_init(int screen_width, int screen_height, gui_renderer_t *renderer)
{
    gui_mgr.screen_width = screen_width;
    gui_mgr.screen_height = screen_height;
    gui_mgr.renderer = *renderer;
    gui_mgr.event_queue = list_create();
    gui_mgr.root = (gui_element_t *)gui_create_panel(0, 0, screen_width, screen_height);
    if (gui_mgr.root) {
        gui_mgr.root->background_color = 0x00000000; // Transparent to show wallpaper
    }
    gui_mgr.hovered_element = NULL;
    gui_mgr.needs_redraw = 1; // Initial draw
    gui_mgr.dirty_rect = (rect_t){0, 0, screen_width, screen_height}; // Full screen dirty
    
    // Initialize background redraw tracking
    gui_mgr.needs_background_redraw = 0;
    gui_mgr.background_dirty_rect = (rect_t){0, 0, 0, 0};
}

// Implement missing functions

void gui_add_element(gui_element_t *parent, gui_element_t *child)
{
    if (parent && child)
    {
        child->parent = parent;
        if (!parent->children) parent->children = list_create();
        list_append(parent->children, child);
        gui_mgr.needs_redraw = 1; // Trigger redraw when elements change
    }
}

// Window Button Constants
#define TITLE_BAR_HEIGHT 24
#define BTN_SIZE 16
#define BTN_MARGIN 4

// Window Button Constants (Updated for Hit Testing)
#define TITLE_BAR_HEIGHT 24
#define BTN_SIZE 16
#define BTN_MARGIN 4

// Draw a window with decorations (Title bar, Close/Min/Max buttons)
// Draw a window with decorations (Title bar, Close/Min/Max buttons)
// Draw a window with decorations (Title bar, Close/Min/Max buttons)
void gui_bring_to_front(gui_element_t *element) {
    if (!element || !element->parent) return;
    
    list_t *list = element->parent->children;
    if (!list || list->size < 2) return;
    
    // Check if already last (tail)
    if (list->tail && list->tail->data == element) return;
    
    // Find node
    list_node_t *node = list->head;
    while (node) {
        if (node->data == element) {
            // Remove from current position
            list_remove_node(list, node);
            // Append to end (top)
            list_append(list, element);
            gui_mgr.needs_redraw = 1;
            return;
        }
        node = node->next;
    }
}

// Draw a window with decorations
void gui_draw_window(gui_renderer_t *renderer, gui_element_t *element)
{
    if (!element || !renderer) return;

    gui_window_t *win = (gui_window_t *)element;
    rect_t bounds = element->bounds;
    
    // Theme Colors
    theme_t *theme = theme_get_current();
    uint32_t bg_color = theme->window_bg;
    uint32_t border_col = theme->window_border;
    
    // Traffic Lights (Standard macOS colors, usually fixed)
    uint32_t red = 0xFFFF5F56;
    uint32_t yellow = 0xFFFFBD2E;
    uint32_t green = 0xFF27C93F;

    int radius = 10; 
    int title_h = 30; 
    
    // 1. Draw Main Body with Rounded Corners
    draw_rounded_rect_filled(bounds, radius, bg_color);
    
    // 2. Window Border (Optional, if theme defines it)
    // draw_rounded_rect_outline(bounds, radius, border_col); 

    // 3. Title Separator
    int content_y = bounds.y + title_h;
    draw_line(bounds.x, content_y, bounds.x + bounds.width, content_y, border_col);
    
    // 3. Traffic Lights (RIGHT Aligned, Green-Yellow-Red order)
    // Vertical center of title bar = y + title_h / 2
    int btn_y = bounds.y + (title_h / 2);
    int spacing = 20;
    
    // Start drawing from Right Edge
    // Order: [Empty] [Green] [Yellow] [Red] [Edge]
    // Red (Close): bounds.width - 15 - 12 (offset to center from edge padding? No, 15 is padding)
    // Let's optimize.
    // Rightmost button center: x + width - 15
    // Middle button center:    x + width - 15 - 20
    // Leftmost button center:  x + width - 15 - 40
    
    int right_margin = 20;
    int red_x = bounds.x + bounds.width - right_margin;
    int yellow_x = red_x - spacing;
    int green_x = yellow_x - spacing;
    
    draw_circle_filled(green_x, btn_y, 6, green);      // Maximize (Leftmost of group)
    draw_circle_filled(yellow_x, btn_y, 6, yellow);    // Minimize (Middle)
    draw_circle_filled(red_x, btn_y, 6, red);          // Close (Rightmost)
    
    // 4. Title Text (Centered)
    if (win->title) {
        int text_len = strlen(win->title) * 8; 
        int text_x = bounds.x + (bounds.width - text_len) / 2;
        int text_y = bounds.y + ((title_h - 10) / 2); // 10px font?
        draw_text(win->title, text_x, text_y, 0x404040, 12);
    }
    
    // 5. Draw Tabs (Mac-like Segmented Control or Safari Tabs style)
    if (win->tabs && win->tabs->size > 0) {
        int tab_bar_y = bounds.y + title_h;
        int tab_h = 24;
        
        // Draw tab bar background
        draw_rect_filled((rect_t){bounds.x, tab_bar_y, bounds.width, tab_h}, 0xF0F0F0);
        draw_line(bounds.x, tab_bar_y + tab_h, bounds.x + bounds.width, tab_bar_y + tab_h, 0xD0D0D0);
        
        int tab_x = bounds.x;
        int tab_w = bounds.width / win->tabs->size; // Simple equal width
        
        list_node_t *node = win->tabs->head;
        while(node) {
            gui_tab_t *tab = (gui_tab_t*)node->data;
            int is_active = (win->active_tab == tab);
            
            // Tab Background
            if (is_active) {
                draw_rect_filled((rect_t){tab_x, tab_bar_y + 2, tab_w, tab_h - 2}, 0xFFFFFF); // Integrated 
                // Top accent or active indicator?
            } else {
                draw_rect_filled((rect_t){tab_x, tab_bar_y + 2, tab_w, tab_h - 2}, 0xE0E0E0);
                // Vertical separator
                draw_line(tab_x + tab_w - 1, tab_bar_y + 4, tab_x + tab_w - 1, tab_bar_y + tab_h - 4, 0xC0C0C0);
            }
            
            // Tab Title
            if (tab->title) {
                int tlen = strlen(tab->title) * 8;
                int tx = tab_x + (tab_w - tlen) / 2;
                int ty = tab_bar_y + 6;
                draw_text(tab->title, tx, ty, is_active ? 0x000000 : 0x606060, 12);
            }
            
            tab_x += tab_w;
            node = node->next;
        }
    }
    
    // 6. Draw Children (Content) being clipped to window bounds? 
    // For now simple recursion.
    if (element->children) {
        list_node_t *child_node = element->children->head;
        while (child_node) {
             gui_element_t *child = (gui_element_t*)child_node->data;
             if (child->draw) {
                 child->draw(renderer, child);
             }
             child_node = child_node->next;
        }
    }
}



void gui_window_event_handler(gui_element_t *element, gui_event_t *event)
{
    gui_window_t *win = (gui_window_t *)element;
    int tabs_area_height = (win->tabs && win->tabs->size > 0) ? 24 : 0;
    
    // Always bring to front on any interaction
    if (event->type == GUI_EVENT_MOUSE_DOWN) {
        gui_bring_to_front(element);
    }
    
    // Handle Window Controls (Traffic Lights) - RIGHT SIDE - Priority over drag
    if (event->type == GUI_EVENT_MOUSE_DOWN) {
        int x = event->mouse.pos.x;
        int y = event->mouse.pos.y;
        
        int title_h = 30;
        int btn_y = win->base.bounds.y + (title_h / 2);
        int spacing = 20;

        // Right Aligned Coords
        int right_margin = 20;
        int red_x = win->base.bounds.x + win->base.bounds.width - right_margin;
        int yellow_x = red_x - spacing;
        int green_x = yellow_x - spacing;
        
        // Check if click is in title bar area
        if (y >= win->base.bounds.y && y < win->base.bounds.y + title_h) {
            
            // 1. Close Button (Red) - Rightmost
            int dx = x - red_x;
            int dy = y - btn_y;
            if (dx*dx + dy*dy <= 36) { 
                 // Close Window
                 // Dispatch Close Event
                 if (win->base.event_handler) {
                     gui_event_t close_ev;
                     close_ev.type = GUI_EVENT_WINDOW_CLOSE;
                     win->base.event_handler((gui_element_t*)win, &close_ev);
                 }
                 
                 gui_invalidate_rect(win->base.bounds); // Mark area as dirty BEFORE moving
                 win->base.bounds.x = -1000; // Hide offscreen
                 gui_mgr.needs_redraw = 1;

                 // Also hide children (Terminal Panel) manually since we don't have recursive hiding yet
                 return;
            }
            
            // 2. Minimize Button (Yellow) - Middle
            dx = x - yellow_x;
            if (dx*dx + dy*dy <= 36) {
                 // Minimize (hide offscreen but mark as minimized)
                 if (!win->is_minimized) {
                     gui_invalidate_rect(win->base.bounds); // Mark area as dirty
                     win->is_minimized = 1;
                     win->saved_bounds = win->base.bounds; // Save position
                     win->base.bounds.x = -1000; // Hide
                     gui_mgr.needs_redraw = 1;
                 }
                 return;
            }
            
            // 3. Maximize Button (Green) - Leftmost of group
            dx = x - green_x;
            if (dx*dx + dy*dy <= 36) {
                 if (!win->is_maximized) {
                      // Save current bounds
                      win->saved_bounds = win->base.bounds;
                      
                      // Maximize to fill screen (leave space for top bar and dock)
                      // Top bar: 24px
                      // Dock: ~90px (70px height + 20px margin)
                      win->base.bounds.x = 0;
                      win->base.bounds.y = 24; // Below top bar
                      win->base.bounds.width = gui_mgr.screen_width;
                      win->base.bounds.height = gui_mgr.screen_height - 24 - 90; // Top bar + dock space
                      win->is_maximized = 1;
                      
                      // Resize child content
                      if (win->tabs) {
                          list_node_t *node = win->tabs->head;
                          while(node) {
                              gui_tab_t *tab = (gui_tab_t*)node->data;
                              if(tab->content) {
                                   int content_h = win->base.bounds.height - (TITLE_BAR_HEIGHT + 24);
                                   tab->content->bounds.width = win->base.bounds.width;
                                   tab->content->bounds.height = content_h;
                              }
                              node = node->next;
                          }
                      }
                      
                      gui_invalidate_rect(win->base.bounds);
                  } else {
                      // Restore
                      win->base.bounds = win->saved_bounds;
                      win->is_maximized = 0;
                      gui_invalidate_rect(win->base.bounds);
                  }
                  gui_mgr.needs_redraw = 1;
                  return;
             }
         }
     }

    // Dragging Logic
    if (event->type == GUI_EVENT_MOUSE_DOWN) {
        // Allow dragging if clicked in Title Bar OR Tab Bar (if unused by tab click)
        int safe_drag_y = win->base.bounds.y + TITLE_BAR_HEIGHT + tabs_area_height;
        
        if (event->mouse.pos.y < safe_drag_y) {
            // Start Dragging (if in title bar and not a button)
            if (!win->is_maximized) {
                win->is_dragging = 1;
                win->drag_offset.x = event->mouse.pos.x - win->base.bounds.x;
                win->drag_offset.y = event->mouse.pos.y - win->base.bounds.y;
                
                // CAPTURE MOUSE: Receive events even if cursor leaves window
                gui_capture_mouse((gui_element_t*)win);
            }
        }
    }
    else if (event->type == GUI_EVENT_MOUSE_UP) {
        if (win->is_dragging) {
            win->is_dragging = 0;
            // RELEASE MOUSE: Stop capturing
            gui_release_mouse();
        }
    }
    else if (event->type == GUI_EVENT_MOUSE_MOVE) {
        if (win->is_dragging && !win->is_maximized) {
            int new_x = event->mouse.pos.x - win->drag_offset.x;
            int new_y = event->mouse.pos.y - win->drag_offset.y;
            
            gui_set_position((gui_element_t*)win, new_x, new_y);
            gui_mgr.needs_redraw = 1;
        }
    }
}

gui_window_t *gui_create_window(const char *title, int x, int y, int width, int height)
{
    gui_window_t *win = (gui_window_t *)memory_alloc(sizeof(gui_window_t));
    if (!win) return NULL;
    
    win->base.type = GUI_ELEMENT_WINDOW;
    win->base.state = GUI_STATE_NORMAL;
    win->base.bounds.x = x;
    win->base.bounds.y = y;
    win->base.bounds.width = width;
    win->base.bounds.width = width;
    win->base.bounds.height = height;
    win->base.flags = 0; // Initialize flags (Ensure not HIDDEN by garbage)
    
    // Copy title using manual logic since no strdup
    int len = 0; while(title[len]) len++;
    win->title = (char*)memory_alloc(len + 1);
    char* d = win->title; const char* s = title; while((*d++ = *s++));
    
    win->base.text = win->title; 
    win->base.parent = NULL;
    win->base.children = list_create();
    
    // Init state
    win->is_dragging = 0;
    win->is_maximized = 0;
    win->is_minimized = 0;
    win->saved_bounds = win->base.bounds;
    
    win->tabs = list_create();
    win->active_tab = NULL;
    
    // Userspace Init
    if (current_process) {
        win->owner_pid = current_process->pid;
    } else {
        win->owner_pid = 0; // Kernel
    }
    win->incoming_events = list_create();

    win->base.event_handler = gui_window_event_handler;
    win->base.draw = gui_draw_window;
    
    win->base.background_color = 0xFFFFFFFF;
    win->base.border_color = 0xFF000000;
    
    // Hook: Automatically manage new windows with WM
    wm_manage_window(win);
    
    return win;
}

gui_label_t *gui_create_label(const char *text, int x, int y, int width, int height)
{
    gui_label_t *label = (gui_label_t *)memory_alloc(sizeof(gui_label_t));
    if (!label) return NULL;
    
    // Base Init
    label->base.type = GUI_ELEMENT_LABEL;
    label->base.state = GUI_STATE_NORMAL;
    label->base.bounds.x = x;
    label->base.bounds.y = y;
    label->base.bounds.width = width;
    label->base.bounds.height = height;
    
    // Copy Text
    if (text) {
        int len = 0; while(text[len]) len++;
        label->base.text = (char*)memory_alloc(len + 1);
        char* d = label->base.text; const char* s = text; while((*d++ = *s++));
    } else {
        label->base.text = NULL;
    }
    
    label->base.parent = NULL;
    label->base.children = NULL; // Labels usually have no children, but list_create() if needed
    label->base.event_handler = gui_default_event_handler;
    label->base.draw = gui_draw_element_default;
    
    // Transparent background usually for labels, or inherit?
    // Let's assume transparent if 0, but default handler draws BG.
    // Set to parent color or transparent? 
    // Types.h doesn't support alpha in u32 color easily unless renderer handles it.
    // Use a specific color for now, e.g., 0xE0E0E0 matches start menu.
    label->base.background_color = 0xE0E0E0; 
    label->base.text_color = 0x000000;
    
    return label;
}

void gui_remove_element(gui_element_t *element) {
    if (!element || !element->parent) return;
    list_t *list = element->parent->children;
    
    list_node_t *node = list->head;
    while(node) {
        if (node->data == element) {
             list_remove_node(list, node);
             element->parent = NULL; // Detach
             gui_mgr.needs_redraw = 1;
             return;
        }
        node = node->next;
    }
}

// Helper function to recursively update child positions without invalidation
static void gui_update_child_positions_recursive(gui_element_t *element, int dx, int dy) {
    if (!element || !element->children) return;
    
    list_node_t *node = element->children->head;
    while(node) {
        gui_element_t *child = (gui_element_t*)node->data;
        child->bounds.x += dx;
        child->bounds.y += dy;
        // Recursively update grandchildren
        gui_update_child_positions_recursive(child, dx, dy);
        node = node->next;
    }
}

void gui_set_position(gui_element_t *element, int x, int y) {
    if (!element) return;
    
    int dx = x - element->bounds.x;
    int dy = y - element->bounds.y;
    
    // Early exit if no movement
    if (dx == 0 && dy == 0) return;
    
    // Calculate union of old and new bounds for single invalidation
    rect_t old_bounds = element->bounds;
    
    element->bounds.x = x;
    element->bounds.y = y;
    
    // Move all children recursively
    gui_update_child_positions_recursive(element, dx, dy);
    
    // CRITICAL: Mark background for redraw at old position
    // This ensures wallpaper shows through instead of artifacts
    if (element->type == GUI_ELEMENT_WINDOW) {
        gui_mgr.needs_background_redraw = 1;
        if (gui_mgr.background_dirty_rect.width == 0) {
            gui_mgr.background_dirty_rect = old_bounds;
        } else {
            gui_mgr.background_dirty_rect = util_rect_union(gui_mgr.background_dirty_rect, old_bounds);
        }
    }
    
    // Invalidate union of old and new positions (covers both element and all children)
    gui_invalidate_rect(util_rect_union(old_bounds, element->bounds));
}


void gui_window_add_tab(gui_window_t *window, const char *title, gui_element_t *content) {
    if (!window || !title) return;
    
    gui_tab_t *tab = (gui_tab_t*)memory_alloc(sizeof(gui_tab_t));
    if (!tab) return;
    
    // Copy Title
    int len = 0; while(title[len]) len++;
    tab->title = (char*)memory_alloc(len + 1);
    char* d = tab->title; const char* s = title; while((*d++ = *s++));
    
    tab->content = content;
    tab->next = NULL;
    
    // Add to list
    list_append(window->tabs, tab);
    
    // If first tab, activate it
    if (!window->active_tab) {
        window->active_tab = tab;
        if (content) {
            // Adjust content position?
            int content_y_offset = TITLE_BAR_HEIGHT + 24; // Title + Tab bar
             gui_set_position(content, window->base.bounds.x + 2, window->base.bounds.y + content_y_offset);
             
            gui_add_element((gui_element_t*)window, content);
        }
    }
    
    gui_mgr.needs_redraw = 1;
}
