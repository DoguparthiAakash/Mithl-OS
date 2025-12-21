#include "file_manager.h"
#include "gui.h"
#include "graphics.h"
#include "string.h"
#include "memory.h"
#include "vfs.h"
#include "console.h"
#include "mouse.h"
#include <bootlogo.h> // For icons (using logo as placeholder for now or generic squares)

#define FM_BG_COLOR 0xFFF0F0F0
#define FM_SIDEBAR_COLOR 0xFFE0E0E0
#define FM_HEADER_COLOR 0xFFFFFFFF
#define FM_SELECTION_COLOR 0xFFCCE8FF
#define FM_TEXT_COLOR 0xFF000000

// Internal State
static struct {
    char current_path[256];
    gui_window_t* window;
    int selected_index;
    int scroll_offset;
    
    // Cached file list
    struct dirent entry_cache[64];
    int entry_count;
} fm_state;

// Forward declarations
void fm_refresh(void);

// --- Logic ---

void file_manager_init(void) {
    memset(&fm_state, 0, sizeof(fm_state));
    strcpy(fm_state.current_path, "/home/aakash");
    fm_state.selected_index = -1;
}

// Read directory content
void fm_refresh(void) {
    fs_node_t *node = vfs_resolve_path(fm_state.current_path);
    if (!node || (node->flags & 0x7) != FS_DIRECTORY) {
        // Fallback to root if path invalid
        strcpy(fm_state.current_path, "/");
        node = fs_root;
    }
    
    fm_state.entry_count = 0;
    int index = 0;
    while (index < 64) {
        struct dirent *d = readdir_fs(node, index);
        if (!d) break;
        
        memcpy(&fm_state.entry_cache[index], d, sizeof(struct dirent));
        fm_state.entry_count++;
        index++;
    }
    fm_state.selected_index = -1;
    gui_mgr.needs_redraw = 1;
}

void file_manager_open_directory(const char* path) {
    // Resolve relative vs absolute?
    // Simplified: assume absolute if starts with /
    if (path[0] == '/') {
        strcpy(fm_state.current_path, path);
    } else {
        // Append? (Logic for relative paths not needed for quick access mostly)
    }
    fm_refresh();
}

void fm_navigate_up(void) {
    if (strcmp(fm_state.current_path, "/") == 0) return;
    
    // Truncate last segment
    char *last_slash = NULL;
    char *p = fm_state.current_path;
    while (*p) {
        if (*p == '/') last_slash = p;
        p++;
    }
    
    if (last_slash && last_slash != fm_state.current_path) {
        *last_slash = 0; // Cut off
    } else {
        // Root
        fm_state.current_path[1] = 0; 
    }
    fm_refresh();
}

void fm_create_file(const char* name) {
    // Basic "Touch" logic using VFS (Requires writable FS)
    // For now, assume VFS supports 'create' or just mock update for UI
    // Real impl: vfs_create_file(fm_state.current_path, name, 0666);
    // Since VFS write isn't fully exposed via simple API yet, we use a placeholder or RAMFS hook
    
    // Mock: Re-read (if implementation existed)
    // For demo interactivity:
    // vfs_open(..., O_CREAT) logic would go here
    fm_refresh();
}

// --- Drawing ---

static void draw_folder_icon(int x, int y) {
    draw_rect((rect_t){x, y, 48, 36}, 0xFFFFD700); // Gold folder
    draw_rect((rect_t){x, y, 20, 10}, 0xFFFFD700); // Tab
}

static void draw_file_icon(int x, int y) {
    draw_rect((rect_t){x+10, y, 28, 40}, 0xFFFFFFFF); // White paper
    draw_rect_outline(x+10, y, 28, 40, 0xFFCCCCCC);
}

static void fm_draw_content(gui_renderer_t *renderer, gui_element_t *element) {
    (void)renderer;
    gui_window_t* win = (gui_window_t*)element;
    int content_w = win->base.bounds.width;
    int content_h = win->base.bounds.height - 24; // Minus title bar
    int start_y = win->base.bounds.y + 24;
    int start_x = win->base.bounds.x;
    
    // 1. Sidebar (Left) - 150px
    draw_rect((rect_t){start_x, start_y, 150, content_h}, FM_SIDEBAR_COLOR);
    
    // Sidebar Items
    int sb_y = start_y + 10;
    draw_text_sf_mono("Quick Access", start_x + 10, sb_y, FM_TEXT_COLOR); sb_y += 24;
    draw_text_sf_mono("  Desktop", start_x + 10, sb_y, FM_TEXT_COLOR); sb_y += 20;
    draw_text_sf_mono("  Downloads", start_x + 10, sb_y, FM_TEXT_COLOR); sb_y += 20;
    draw_text_sf_mono("  Documents", start_x + 10, sb_y, FM_TEXT_COLOR); sb_y += 20;
    
    sb_y += 10;
    draw_text_sf_mono("This PC", start_x + 10, sb_y, FM_TEXT_COLOR); sb_y += 24;
    draw_text_sf_mono("  ZFS Root", start_x + 10, sb_y, 0xFF0088CC); // Blue for ZFS
    
    // 2. Header (Top)
    int main_x = start_x + 150;
    int main_w = content_w - 150;
    draw_rect((rect_t){main_x, start_y, main_w, 40}, FM_HEADER_COLOR);
    draw_rect_outline(main_x, start_y + 39, main_w, 1, 0xFFDDDDDD); // Divider
    
    // Buttons: New, Up
    // "New +" Button
    draw_rect((rect_t){main_x + 10, start_y + 8, 60, 24}, 0xFF0078D4); // Win11 Blue
    draw_text_sf_mono("New +", main_x + 18, start_y + 12, 0xFFFFFFFF);
    
    // "Up" Button
    draw_rect((rect_t){main_x + 80, start_y + 8, 30, 24}, 0xFFDDDDDD);
    draw_text_sf_mono("^", main_x + 91, start_y + 12, FM_TEXT_COLOR);
    
    // Path Bar
    draw_rect((rect_t){main_x + 120, start_y + 8, main_w - 130, 24}, 0xFFF9F9F9);
    draw_rect_outline(main_x + 120, start_y + 8, main_w - 130, 24, 0xFFDDDDDD);
    draw_text_sf_mono(fm_state.current_path, main_x + 125, start_y + 12, 0xFF555555);
    
    // 3. File Grid
    int grid_y = start_y + 40;
    int grid_h = content_h - 40 - 20; // -Header -StatusBar
    draw_rect((rect_t){main_x, grid_y, main_w, grid_h}, FM_BG_COLOR);
    
    int item_w = 80;
    int item_h = 80;
    int cols = main_w / item_w;
    if (cols < 1) cols = 1;
    
    for (int i = 0; i < fm_state.entry_count; i++) {
        int row = i / cols;
        int col = i % cols;
        
        int ix = main_x + col * item_w + 10;
        int iy = grid_y + row * item_h + 10;
        
        // Selection
        if (i == fm_state.selected_index) {
            draw_rect((rect_t){ix - 5, iy - 5, item_w - 10, item_h - 10}, FM_SELECTION_COLOR); // Highlight
        }
        
        // Icon
        struct dirent *d = &fm_state.entry_cache[i];
        // Note: d_type isn't always reliable in simple FS, but assume 0=file, 1=dir? 
        // Need to check vfs implementation. Usually we checked node->flags.
        // For cached dirent, we might not have flags properly. 
        // Let's guess based on name (no ext -> folder?) or fallback.
        // Wait, readdir_fs usually returns just name/ino. Flags require resolving.
        // OPTIMIZATION: Just assume no-extension or common folders are folders.
        
        // Simple logic:
        draw_file_icon(ix + 10, iy); // Default file
        // Label
        draw_text_sf_mono(d->name, ix, iy + 50, FM_TEXT_COLOR);
    }
    
    // 4. Status Bar
    int status_y = start_y + content_h - 20;
    draw_rect((rect_t){main_x, status_y, main_w, 20}, 0xFFF5F5F5);
    draw_text_sf_mono("ZFS Compression: lz4 | Dedup: off", main_x + 5, status_y + 4, 0xFF888888);
}

// --- Interaction ---

void fm_handle_event(gui_element_t* element, gui_event_t* event) {
    if (event->type == GUI_EVENT_MOUSE_DOWN) {
        gui_window_t* win = (gui_window_t*)element;
        int mx = event->mouse.pos.x;
        int my = event->mouse.pos.y;
        
        int start_x = win->base.bounds.x;
        int start_y = win->base.bounds.y + 24;
        int main_x = start_x + 150;
        
        // Handle Header Clicks
        if (my >= start_y && my < start_y + 40 && mx > main_x) {
            // New Button (approx coords)
            if (mx >= main_x + 10 && mx <= main_x + 70) {
                 // Create dummy file logic
                 // For now, create a test file in current dir
                 // Need fs_create_file or similar.
                 // Mock:
                 int exists = 0;
                 // (Check if file exists logic...)
                 if (!exists) {
                      // fs_write_file(cat path + "/new.txt", "New", 3);
                      // Since we lack robust write API in user-space code here without FS headers fully included:
                      // We will Trigger a Refresh to simulate.
                      fm_refresh(); 
                 }
            }
            // Up Button
            if (mx >= main_x + 80 && mx <= main_x + 110) {
                fm_navigate_up();
            }
        }
        
        // Handle Grid Clicks
        int grid_y = start_y + 40;
        if (my > grid_y && mx > main_x) {
            int item_w = 80;
            int item_h = 80;
            int cols = (win->base.bounds.width - 150) / item_w;
            if (cols < 1) cols = 1;
            
            int rx = mx - main_x - 10;
            int ry = my - grid_y - 10;
            if (rx < 0) rx = 0; if (ry < 0) ry = 0;
            
            int col = rx / item_w;
            int row = ry / item_h;
            
            int idx = row * cols + col;
            
            if (idx < fm_state.entry_count) {
                // Double Click Logic? (Requires timestamps or state)
                // Simplified: Select, if already selected -> Open
                if (fm_state.selected_index == idx) {
                    // Open!
                    char path[256];
                    strcpy(path, fm_state.current_path);
                    if (path[strlen(path)-1] != '/') strcat(path, "/"); // Ensure slash
                    strcat(path, fm_state.entry_cache[idx].name);
                    
                    // Check if dir or file?
                    fs_node_t *node = vfs_resolve_path(path);
                    if (node && (node->flags & 0x7) == FS_DIRECTORY) {
                         file_manager_open_directory(path);
                    } else {
                         // Open file (maybe separate app?)
                         // Launch Text Editor
                         // text_editor_open(path);
                    }
                } else {
                    fm_state.selected_index = idx;
                    gui_mgr.needs_redraw = 1;
                }
            } else {
                fm_state.selected_index = -1;
                gui_mgr.needs_redraw = 1;
            }
        }
        
        // Sidebar Clicks
        if (mx < main_x && my > start_y) {
             // Basic hit testing
             int rel_y = my - start_y;
             if (rel_y > 34 && rel_y < 54) file_manager_open_directory("/home/aakash/Desktop"); // "Desktop"
             if (rel_y > 54 && rel_y < 74) file_manager_open_directory("/home/aakash/Downloads"); // "Downloads"
             if (rel_y > 74 && rel_y < 94) file_manager_open_directory("/home/aakash/Documents"); // "Documents"
             if (rel_y > 118 && rel_y < 138) file_manager_open_directory("/"); // "ZFS Root"
        }
    }
}

void file_manager_show(void) {
    if (!fm_state.window) {
        fm_state.window = gui_create_window("File Manager", 100, 100, 700, 500);
        fm_state.window->base.draw = fm_draw_content;
        fm_state.window->base.event_handler = fm_handle_event;
        
        file_manager_init();
        fm_refresh();
        
        gui_add_element(gui_mgr.root, (gui_element_t*)fm_state.window);
    }
    
    // Bring to front and restore if minimized
    if (fm_state.window->base.bounds.x < 0 && fm_state.window->base.bounds.x > -2000) { 
        // Just hidden
        fm_state.window->base.bounds.x = 100; // Restore default or saved?
    }
    
    gui_bring_to_front((gui_element_t*)fm_state.window);
    fm_state.window->base.flags &= ~GUI_FLAG_HIDDEN;
    gui_mgr.needs_redraw = 1;
}
