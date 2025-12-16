#include "file_manager.h"
#include "filesystem.h"
#include "icons_data.h"
#include "gui.h"
#include "memory.h"
#include "string.h"
#include "vfs.h"
#include "apps/text_editor/text_editor.h"

static gui_window_t *fm_window = NULL;
static char current_path[MAX_FILENAME];
static int selected_file_index = -1;

#define SIDEBAR_W 150

// Forward declarations
static void fm_draw_content(gui_renderer_t *renderer, gui_element_t *element);
static void fm_handle_event(gui_element_t *element, gui_event_t *event);

void file_manager_init(void) {
    if (fm_window) return; // Already initialized

    // Create window
    fm_window = gui_create_window("File Manager", 150, 150, 600, 400);
    if (!fm_window) return;

    // We do NOT override window draw/event anymore, so it uses default window implementation (which supports tabs).
    // fm_window->base.background_color = 0xFFFFFF; // Default is fine

    // Create Local Tab Content
    gui_panel_t *local_panel = gui_create_panel(0, 0, 600, 350); // Size will be adjusted by layout?
    local_panel->base.draw = fm_draw_content;
    local_panel->base.event_handler = fm_handle_event;
    local_panel->base.background_color = 0xFFFFFF; // White bg for content
    
    // Create Network Tab Content (Placeholder)
    gui_panel_t *network_panel = gui_create_panel(0, 0, 600, 350);
    network_panel->base.background_color = 0xF0F0F0; // Differt bg
    // use default draw (rect) or custom? Let's use default to show empty panel
    
    // Set initial path
    strcpy(current_path, "/");
    selected_file_index = -1;

    // Add Tabs
    gui_window_add_tab(fm_window, "Local", (gui_element_t*)local_panel);
    gui_window_add_tab(fm_window, "Network", (gui_element_t*)network_panel);

    // Add Window to Root
    gui_add_element(gui_mgr.root, (gui_element_t*)fm_window);
}

void file_manager_show(void) {
    if (!fm_window) {
        file_manager_init();
    }
    // Bring to front (simple focus for now)
    if (fm_window) {
        gui_mgr.focused_element = (gui_element_t*)fm_window;
        // Reset to root if needed or keep state
    }
}

void file_manager_open_directory(const char* path) {
    // For this simple FS, we only have root directories logic or flat?
    // filesystem.h shows directories array and root.
    // Let's assume flat or simple traversal for now.
    // We'll just update current path stub.
    strcpy(current_path, path);
    // Refresh?
}

static void fm_draw_content(gui_renderer_t *renderer, gui_element_t *element) {
    // 1. Draw Background
    renderer->draw_rect(element->bounds, element->background_color); // 0xFFFFFF
    
    rect_t bounds = element->bounds;
    // content_y is relative to element bounds. 
    // Since this is a panel INSIDE the window, bounds.y is already offset by title bar + tab bar.
    // So we start drawing at bounds.y
    
    // So we start drawing at bounds.y
    int content_y = bounds.y;
    int content_h = bounds.height;
    
    // 2. Draw Sidebar (Left)
    rect_t sidebar_rect = {bounds.x, content_y, SIDEBAR_W, content_h - 10}; // -10 for rounded corner clipping approx
    // We rely on standard drawing, but to look good we might need to be careful not to draw over rounded corners at bottom left.
    // Ideally we clip. For now, just draw a rect.
    draw_rect_filled(sidebar_rect, 0xF6F6F6); // Slightly darker than white
    
    // Sidebar Separator
    draw_rect_filled((rect_t){bounds.x + SIDEBAR_W, content_y, 1, content_h}, 0xE0E0E0);

    // Sidebar Items
    int sb_item_y = content_y + 20;
    int sb_indent = bounds.x + 15;
    
    // Header "Favorites"
    draw_text("Favorites", sb_indent, sb_item_y, 0x808080, 10);
    sb_item_y += 20;
    
    const char *favs[] = {"AirDrop", "Recents", "Applications", "Desktop", "Documents", "Downloads"};
    for (int i=0; i<6; i++) {
        // Pseudo-icon (small blue dot or similar?)
        draw_rect_filled((rect_t){sb_indent, sb_item_y + 2, 8, 8}, 0xAAAAAA); // Placeholder icon
        draw_text(favs[i], sb_indent + 15, sb_item_y, 0x404040, 12);
        sb_item_y += 24;
    }

    // 3. Main Content Area (Files)
    int start_x = bounds.x + SIDEBAR_W + 20;
    int start_y = content_y + 20;
    int grid_w = 90;
    int grid_h = 100;
    int area_w = bounds.width - SIDEBAR_W;
    int cols_per_row = (area_w - 40) / grid_w;
    if (cols_per_row < 1) cols_per_row = 1;
    
    for (int i = 0; i < fs.root.file_count; i++) {
        int col = i % cols_per_row;
        int row = i / cols_per_row;
        
        int icon_x = start_x + col * grid_w;
        int icon_y = start_y + row * grid_h;
        
        // Clip
        if (icon_y + grid_h > bounds.y + bounds.height) break;
        
        // Selection Box
        if (selected_file_index == i) {
            draw_rounded_rect_filled((rect_t){icon_x - 5, icon_y - 5, 80, 80}, 8, 0xE0E0FF); 
        }
        
        // Draw Icon
        int draw_icon_x = icon_x + (grid_w - 48) / 2 - 15; 
        
        if (fs.root.files[i].type == FILE_TYPE_DIRECTORY) {
            draw_icon(draw_icon_x, icon_y, icon_folder);
        } else {
             draw_icon(draw_icon_x, icon_y, icon_file);
        }
        
        // Draw Filename
        int text_len = strlen(fs.root.files[i].name) * 8; 
        int text_x = icon_x + (grid_w - text_len) / 2 - 15;
        if (text_x < icon_x) text_x = icon_x; // clamp left
        
        draw_text(fs.root.files[i].name, text_x, icon_y + 55, 0x000000, 12);
    }
}

static void fm_handle_event(gui_element_t *element, gui_event_t *event) {
    // This is now a panel handler, not window handler.
    // Standard checks
    gui_default_event_handler(element, event);

    rect_t bounds = element->bounds;

    if (event->type == GUI_EVENT_MOUSE_DOWN) {
        int x = event->mouse.pos.x;
        int y = event->mouse.pos.y;
        
        // No title bar check needed here, as we are inside the content panel.
        if (!point_in_rect(event->mouse.pos, bounds)) return;
        
        // Offset for Inner Content
        // Our draw logic uses content_y = bounds.y.
        // Icons start at:
        int start_x = bounds.x + SIDEBAR_W + 20;
        int start_y = bounds.y + 20; 

        // Icons
        int grid_w = 90;
        int grid_h = 100;
        int cols_per_row = (bounds.width - 40) / grid_w;
        if (cols_per_row < 1) cols_per_row = 1;
        
        for (int i = 0; i < fs.root.file_count; i++) {
            int col = i % cols_per_row;
            int row = i / cols_per_row;
            
            int icon_x = start_x + col * grid_w;
            int icon_y = start_y + row * grid_h;
            
            // Check approximate hit (icon + text area)
            if (x >= icon_x && x < icon_x + grid_w && y >= icon_y && y < icon_y + grid_h) {
                if (selected_file_index == i) {
                    // Double click logic
                    file_entry_t *f = &fs.root.files[i];
                    if (f->type == FILE_TYPE_TEXT) {
                         text_editor_open(f->name);
                    }
                }
                selected_file_index = i;
                gui_mgr.needs_redraw = 1; // update highlight
                return;
            }
        }
        
        if (selected_file_index != -1) {
            selected_file_index = -1;
            gui_mgr.needs_redraw = 1;
        }
    }
}
