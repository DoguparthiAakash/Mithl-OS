#include "file_manager.h"
#include "gui.h"
#include "graphics.h"
#include "string.h"
#include "memory.h"
#include "vfs.h"
#include "console.h"
#include "mouse.h"
#include <bootlogo.h>
#include "ports.h" 
#include <wm.h>

// Include Generated Kora Icons
#include "icons/kora_folder.h"
#include "icons/kora_file.h"
#include "icons/kora_home.h"
#include "icons/kora_desktop.h"
#include "icons/kora_documents.h"
#include "icons/kora_downloads.h"
#include "icons/kora_music.h"
#include "icons/kora_pictures.h"
#include "icons/kora_videos.h"


#define FM_BG_COLOR 0xFFFFFFFF
#define FM_SIDEBAR_COLOR 0xFFF3F3F3
#define FM_HEADER_COLOR 0xFFF9F9F9
#define FM_SELECTION_COLOR 0xFFCCE8FF
#define FM_TEXT_COLOR 0xFF202020

// Internal State type
typedef struct {
    char name[128];
    int type;
} fm_entry_t;

// Clipboard state
static struct {
    char path[256];
    int operation;  // 0=none, 1=copy, 2=cut
} clipboard = {0};

static struct {
    char current_path[256];
    gui_window_t* window;
    gui_panel_t* panel;
    int selected_index;
    int scroll_offset;
    uint32_t last_click_time;
    int last_click_index;
    
    fm_entry_t entry_cache[64];
    int entry_count;
} fm_state;

// Forward declarations
void fm_refresh(void);
void fm_navigate_up(void);

// Action functions
void fm_action_new_file(void);
void fm_action_new_folder(void);
void fm_action_delete(void);
void fm_action_rename(void);
void fm_action_copy(void);
void fm_action_cut(void);
void fm_action_paste(void);
void fm_action_open(int index);

// Helper for strchr
static char* fm_strchr(const char *s, int c) {
    while (*s != (char)c) {
        if (!*s++) {
            return NULL;
        }
    }
    return (char *)s;
}

static void fm_handle_event(gui_element_t* element, gui_event_t* event);
static void fm_draw_sidebar_item(int x, int y, const uint32_t* icon, const char* label, const char* target_path);
static void fm_draw_content(gui_renderer_t* renderer, gui_element_t* element);

// --- Logic ---

void file_manager_init(void) {
    memset(&fm_state, 0, sizeof(fm_state));
    strcpy(fm_state.current_path, "/home/aakash");
    fm_state.selected_index = -1;
}

void fm_refresh(void) {
    serial_write("[FM] Refreshing path: ");
    serial_write(fm_state.current_path);
    serial_write("\n");

    fs_node_t *node = NULL;
    if (strcmp(fm_state.current_path, "/") == 0) {
        node = fs_root;
    } else {
       node = vfs_resolve_path(fm_state.current_path);
    }
    if (!node) {
        serial_write("[FM] Path failed resolution!\n");
        strcpy(fm_state.current_path, "/"); 
        node = fs_root;
    }
    
    if (node && (node->flags & 0x7) != FS_DIRECTORY) {
         serial_write("[FM] Not a directory!\n");
        strcpy(fm_state.current_path, "/");
        node = fs_root;
    }
    
    fm_state.entry_count = 0;
    int index = 0;
    while (index < 64) {
        struct dirent *d = readdir_fs(node, index);
        if (!d) break;
        
        strncpy(fm_state.entry_cache[index].name, d->name, 127);
        fm_state.entry_cache[index].name[127] = 0;
        
        // Get type by finding the child node
        fs_node_t *child = finddir_fs(node, d->name);
        if (child) {
            fm_state.entry_cache[index].type = (child->flags & 0x7);
        } else {
            fm_state.entry_cache[index].type = FS_FILE;
        }
        
        fm_state.entry_count++;
        index++;
    }
    fm_state.selected_index = -1;
    gui_mgr.needs_redraw = 1;
}

void file_manager_open_directory(const char* path) {
    if (path[0] == '/') {
        strcpy(fm_state.current_path, path);
    }
    fm_refresh();
}

void fm_navigate_up(void) {
    if (strcmp(fm_state.current_path, "/") == 0) return;
    
    char *last_slash = NULL;
    char *p = fm_state.current_path;
    while (*p) {
        if (*p == '/') last_slash = p;
        p++;
    }
    
    if (last_slash && last_slash != fm_state.current_path) {
        *last_slash = 0;
    } else {
        strcpy(fm_state.current_path, "/");
    }
    fm_refresh();
}

// --- File Operations ---

/*
// Callback for new file dialog
static void fm_new_file_callback(const char* name) {
    if (!name || strlen(name) == 0) return;
    
    fs_node_t *current = vfs_resolve_path(fm_state.current_path);
    if (current) {
        create_fs(current, (char*)name, 0644);
        fm_refresh();
    }
}

// Callback for new folder dialog
static void fm_new_folder_callback(const char* name) {
    if (!name || strlen(name) == 0) return;
    
    fs_node_t *current = vfs_resolve_path(fm_state.current_path);
    if (current) {
        mkdir_fs(current, (char*)name, 0755);
        fm_refresh();
    }
}

// Callback for rename dialog
static void fm_rename_callback(const char* new_name) {
    if (!new_name || strlen(new_name) == 0 || fm_state.selected_index < 0) return;
    
    fs_node_t *current = vfs_resolve_path(fm_state.current_path);
    if (current) {
        const char* old_name = fm_state.entry_cache[fm_state.selected_index].name;
        vfs_rename(current, old_name, new_name);
        fm_refresh();
    }
}

// Callback for delete confirmation
static void fm_delete_callback(int confirmed) {
    if (!confirmed || fm_state.selected_index < 0) return;
    
    fs_node_t *current = vfs_resolve_path(fm_state.current_path);
    if (current) {
        const char* name = fm_state.entry_cache[fm_state.selected_index].name;
        vfs_delete(current, name);
        fm_refresh();
    }
}
*/

// Action: Create new file
void fm_action_new_file(void) {
    // Note: gui_show_input_dialog requires gui_dialog.h to be included
    // For now, create with default name
    fs_node_t *current = vfs_resolve_path(fm_state.current_path);
    if (current) {
        create_fs(current, "new_file.txt", 0644);
        fm_refresh();
    }
}

// Action: Create new folder
void fm_action_new_folder(void) {
    fs_node_t *current = vfs_resolve_path(fm_state.current_path);
    if (current) {
        mkdir_fs(current, "New Folder", 0755);
        fm_refresh();
    }
}

// Action: Delete selected item
void fm_action_delete(void) {
    if (fm_state.selected_index < 0) return;
    
    // Delete without confirmation for now (can add dialog later)
    fs_node_t *current = vfs_resolve_path(fm_state.current_path);
    if (current) {
        const char* name = fm_state.entry_cache[fm_state.selected_index].name;
        vfs_delete(current, name);
        fm_refresh();
    }
}

// Action: Rename selected item
void fm_action_rename(void) {
    if (fm_state.selected_index < 0) return;
    
    // For now, rename to "renamed_<original>"
    fs_node_t *current = vfs_resolve_path(fm_state.current_path);
    if (current) {
        const char* old_name = fm_state.entry_cache[fm_state.selected_index].name;
        char new_name[128];
        strcpy(new_name, "renamed_");
        strcat(new_name, old_name);
        vfs_rename(current, old_name, new_name);
        fm_refresh();
    }
}

// Action: Copy selected item
void fm_action_copy(void) {
    if (fm_state.selected_index < 0) return;
    
    // Build full path
    strcpy(clipboard.path, fm_state.current_path);
    if (clipboard.path[strlen(clipboard.path) - 1] != '/') {
        strcat(clipboard.path, "/");
    }
    strcat(clipboard.path, fm_state.entry_cache[fm_state.selected_index].name);
    clipboard.operation = 1;  // Copy
    
    serial_write("[FM] Copied: ");
    serial_write(clipboard.path);
    serial_write("\n");
}

// Action: Cut selected item
void fm_action_cut(void) {
    if (fm_state.selected_index < 0) return;
    
    // Build full path
    strcpy(clipboard.path, fm_state.current_path);
    if (clipboard.path[strlen(clipboard.path) - 1] != '/') {
        strcat(clipboard.path, "/");
    }
    strcat(clipboard.path, fm_state.entry_cache[fm_state.selected_index].name);
    clipboard.operation = 2;  // Cut
    
    serial_write("[FM] Cut: ");
    serial_write(clipboard.path);
    serial_write("\n");
}

// Action: Paste from clipboard
void fm_action_paste(void) {
    if (clipboard.operation == 0) return;  // Nothing in clipboard
    
    // Build destination path
    char dest_path[256];
    strcpy(dest_path, fm_state.current_path);
    if (dest_path[strlen(dest_path) - 1] != '/') {
        strcat(dest_path, "/");
    }
    
    // Extract filename from source
    char *last_slash = NULL;
    for (char *p = clipboard.path; *p; p++) {
        if (*p == '/') last_slash = p;
    }
    
    if (last_slash) {
        strcat(dest_path, last_slash + 1);
    }
    
    if (clipboard.operation == 1) {  // Copy
        vfs_copy(clipboard.path, dest_path);
    } else if (clipboard.operation == 2) {  // Cut (move)
        vfs_move(clipboard.path, dest_path);
        clipboard.operation = 0;  // Clear clipboard after move
    }
    
    fm_refresh();
}

// Action: Open file or folder
void fm_action_open(int index) {
    if (index < 0 || index >= fm_state.entry_count) return;
    
    if (fm_state.entry_cache[index].type == FS_DIRECTORY) {
        // Navigate into folder
        if (fm_state.current_path[strlen(fm_state.current_path) - 1] != '/') {
            strcat(fm_state.current_path, "/");
        }
        strcat(fm_state.current_path, fm_state.entry_cache[index].name);
        fm_refresh();
    }
    // For files, we could open in text editor or show preview
}

// --- Drawing ---

static void fm_draw_sidebar_item(int x, int y, const uint32_t* icon, const char* label, const char* target_path) {
    // Check if active (simple check)
    int is_active = (strncmp(fm_state.current_path, target_path, strlen(target_path)) == 0) &&
                    (strlen(fm_state.current_path) == strlen(target_path));
    
    if (is_active) {
        draw_rect_filled((rect_t){x - 10, y - 5, 180, 38}, FM_SELECTION_COLOR);
    }
    
    // Draw ICON
    graphics_draw_image(x, y, 32, 32, icon);
    
    draw_text_sf_mono(label, x + 40, y + 8, FM_TEXT_COLOR);
}


static void fm_draw_content(gui_renderer_t* renderer, gui_element_t* element) {
    (void)renderer; // Unused for now

    int start_x = element->bounds.x;
    int start_y = element->bounds.y;
    int content_w = element->bounds.width;
    int content_h = element->bounds.height;
    
    // Ensure background is clear (Fix Ghosting?)
    // draw_rect_filled(element->bounds, 0xFFFFFFFF); // Clear entire panel first

    // Layout Constants
    int header_h = 50; // Taller header
    int sidebar_w = 200; // Wider sidebar
    int status_h = 24;
    
    // 1. Header Area
    draw_rect_filled((rect_t){start_x, start_y, content_w, header_h}, FM_HEADER_COLOR);
    draw_line(start_x, start_y + header_h, start_x + content_w, start_y + header_h, 0xFFE5E5E5);
    
    int btn_x = start_x + 10;
    int btn_y = start_y + 12;
    
    // Nav Buttons
    draw_rect_outline(btn_x, btn_y, 26, 26, 0xFFCCCCCC);
    draw_text_sf_mono("<", btn_x + 8, btn_y + 5, 0xFF666666);
    
    draw_rect_outline(btn_x + 32, btn_y, 26, 26, 0xFFCCCCCC);
    draw_text_sf_mono("^", btn_x + 40, btn_y + 5, 0xFF666666);
    
    btn_x += 64;
    
    // Address Bar
    int addr_x = btn_x + 10;
    int addr_w = content_w - addr_x - 140; 
    
    draw_rect_outline(addr_x, btn_y, addr_w, 26, 0xFFD0D0D0); 
    draw_rect_filled((rect_t){addr_x + 1, btn_y + 1, addr_w - 2, 24}, 0xFFFFFFFF);
    draw_text_sf_mono(fm_state.current_path, addr_x + 8, btn_y + 5, 0xFF444444);
    
    // Toolbar Buttons (right side of header)
    int toolbar_x = addr_x + addr_w + 10;
    int btn_w = 70;
    int btn_gap = 5;
    
    // New File button
    draw_rect_filled((rect_t){toolbar_x, btn_y, btn_w, 26}, 0xFF0078D4);
    draw_text_sf_mono("File", toolbar_x + 20, btn_y + 5, 0xFFFFFFFF);
    
    // New Folder button
    toolbar_x += btn_w + btn_gap;
    draw_rect_filled((rect_t){toolbar_x, btn_y, btn_w, 26}, 0xFF0078D4);
    draw_text_sf_mono("Folder", toolbar_x + 15, btn_y + 5, 0xFFFFFFFF);
    
    // Delete button
    toolbar_x += btn_w + btn_gap;
    draw_rect_filled((rect_t){toolbar_x, btn_y, btn_w, 26}, 0xFFE81123);
    draw_text_sf_mono("Delete", toolbar_x + 15, btn_y + 5, 0xFFFFFFFF);


    // 2. Sidebar
    int body_y = start_y + header_h;
    int body_h = content_h - header_h - status_h;
    
    draw_rect_filled((rect_t){start_x, body_y, sidebar_w, body_h}, FM_SIDEBAR_COLOR); 
    draw_line(start_x + sidebar_w, body_y, start_x + sidebar_w, body_y + body_h, 0xFFE5E5E5);
    
    int sb_x = start_x + 16;
    int sb_y = body_y + 16;
    
    // Sidebar Items using Kora Icons
    fm_draw_sidebar_item(sb_x, sb_y, kora_home, "Home", "/user/home"); sb_y += 42;
    fm_draw_sidebar_item(sb_x, sb_y, kora_desktop, "Desktop", "/home/aakash/Desktop"); sb_y += 42;
    fm_draw_sidebar_item(sb_x, sb_y, kora_downloads, "Downloads", "/home/aakash/Downloads"); sb_y += 42;
    fm_draw_sidebar_item(sb_x, sb_y, kora_documents, "Documents", "/home/aakash/Documents"); sb_y += 42;
    fm_draw_sidebar_item(sb_x, sb_y, kora_pictures, "Pictures", "/home/aakash/Pictures"); sb_y += 42;
    fm_draw_sidebar_item(sb_x, sb_y, kora_music, "Music", "/home/aakash/Music"); sb_y += 42;
    fm_draw_sidebar_item(sb_x, sb_y, kora_videos, "Videos", "/home/aakash/Videos"); sb_y += 42;


    // 3. Main Content (Grid)
    int main_x = start_x + sidebar_w;
    int main_w = content_w - sidebar_w;
    int grid_y = body_y;
    int grid_h = body_h;
    
    // White Background
    draw_rect_filled((rect_t){main_x, grid_y, main_w, grid_h}, 0xFFFFFFFF); 
    
    int item_w = 90;
    int item_h = 90; // Increased height for text
    int cols = (main_w - 20) / item_w;
    if (cols < 1) cols = 1;

    for (int i = 0; i < fm_state.entry_count; i++) {
        int row = i / cols;
        int col = i % cols;
        
        int ix = main_x + 20 + col * item_w;
        int iy = grid_y + 20 + row * item_h;
        
        // Selection
        if (i == fm_state.selected_index) {
            draw_rect_filled((rect_t){ix - 5, iy - 5, item_w - 10, item_h - 10}, FM_SELECTION_COLOR);
        }
        
        fm_entry_t *d = &fm_state.entry_cache[i];
        
        // Determine Icon
        const uint32_t* icon_to_draw = kora_file;
        if (d->type == FS_DIRECTORY) {
            icon_to_draw = kora_folder;
        } else if (fm_strchr(d->name, '.')) {
             if (strstr(d->name, ".png") || strstr(d->name, ".jpg")) icon_to_draw = kora_pictures;
             else if (strstr(d->name, ".mp3")) icon_to_draw = kora_music;
             else if (strstr(d->name, ".mp4")) icon_to_draw = kora_videos;
             else if (strstr(d->name, ".txt") || strstr(d->name, ".c") || strstr(d->name, ".h")) icon_to_draw = kora_documents;
        }

        // Draw Icon Centered in Item Box
        graphics_draw_image(ix + 24, iy + 10, 32, 32, icon_to_draw);
        
        // Draw Text
        int name_len = strlen(d->name);
        int text_x = ix + (80 - name_len * 9) / 2; // Center text roughly
        if (text_x < ix) text_x = ix;
        
        draw_text_sf_mono(d->name, text_x, iy + 50, FM_TEXT_COLOR);
    }
    
    // 4. Status Bar
    int status_y = start_y + content_h - status_h;
    draw_rect_filled((rect_t){start_x, status_y, content_w, status_h}, 0xFFF9F9F9);
    draw_line(start_x, status_y, start_x + content_w, status_y, 0xFFE5E5E5);
    draw_text_sf_mono("Items: ", start_x + 10, status_y + 6, 0xFF606060);
    
    char buf[16];
    int n = fm_state.entry_count;
    int p = 0;
    if (n==0) { buf[0]='0'; p=1; }
    else {
        char tmp[16]; int t=0;
        while(n>0) { tmp[t++] = (n%10)+'0'; n/=10; }
        while(t>0) buf[p++] = tmp[--t];
    }
    buf[p]=0;
    draw_text_sf_mono(buf, start_x + 60, status_y + 6, 0xFF606060);
}

void fm_handle_event(gui_element_t* element, gui_event_t* event) {
    gui_window_t* win = fm_state.window;
    if (!win) return;
    
    if (event->type == GUI_EVENT_WINDOW_CLOSE) {
        serial_write("[FM] Close event received.\n");
        fm_state.window = NULL;
        fm_state.panel = NULL;
        return;
    }
    
    if (event->type == GUI_EVENT_MOUSE_DOWN) {
        int mx = event->mouse.pos.x;
        int my = event->mouse.pos.y;
        
        serial_write("[FM] Click detected\n");
        
        int start_x = element->bounds.x;
        int start_y = element->bounds.y;
        int header_h = 50;
        int sidebar_w = 200;
        
        // 1. Header Clicks
        if (my < start_y + header_h) {
            int btn_y = start_y + 12;
            
            // Navigation buttons
            if (mx >= start_x + 10 && mx <= start_x + 36 && my >= btn_y && my <= btn_y + 26) { 
                fm_navigate_up(); // Back button
            }
            else if (mx >= start_x + 42 && mx <= start_x + 68 && my >= btn_y && my <= btn_y + 26) {
                fm_navigate_up(); // Up button
            }
            
            // Calculate toolbar button positions (matching draw code)
            int addr_x = start_x + 84;
            int content_w = element->bounds.width;
            int addr_w = content_w - addr_x - 140;
            int toolbar_x = addr_x + addr_w + 10;
            int btn_w = 70;
            int btn_gap = 5;
            
            // New File button
            if (mx >= toolbar_x && mx <= toolbar_x + btn_w && my >= btn_y && my <= btn_y + 26) {
                fm_action_new_file();
                gui_mgr.needs_redraw = 1;
            }
            
            // New Folder button
            toolbar_x += btn_w + btn_gap;
            if (mx >= toolbar_x && mx <= toolbar_x + btn_w && my >= btn_y && my <= btn_y + 26) {
                fm_action_new_folder();
                gui_mgr.needs_redraw = 1;
            }
            
            // Delete button
            toolbar_x += btn_w + btn_gap;
            if (mx >= toolbar_x && mx <= toolbar_x + btn_w && my >= btn_y && my <= btn_y + 26) {
                fm_action_delete();
                gui_mgr.needs_redraw = 1;
            }
        }
        // 2. Sidebar Clicks (Updated logic for new items)
        else if (mx < start_x + sidebar_w) {
             int rel_y = my - (start_y + header_h + 16); // offset from first item
             if (rel_y < 0) return;
             
             int item_h = 42;
             int index = rel_y / item_h;
             
             switch(index) {
                 case 0: file_manager_open_directory("/user/home"); break; // Home
                 case 1: file_manager_open_directory("/home/aakash/Desktop"); break;
                 case 2: file_manager_open_directory("/home/aakash/Downloads"); break;
                 case 3: file_manager_open_directory("/home/aakash/Documents"); break;
                 case 4: file_manager_open_directory("/home/aakash/Pictures"); break;
                 case 5: file_manager_open_directory("/home/aakash/Music"); break;
                 case 6: file_manager_open_directory("/home/aakash/Videos"); break;
             }
        }
        // 3. Grid Clicks
        else {
             int grid_x = start_x + sidebar_w;
             int grid_y = start_y + header_h;
             int item_w = 90;
             int item_h = 90;
             int grid_w = element->bounds.width - sidebar_w;
             int cols = (grid_w - 20) / item_w;
             if (cols < 1) cols = 1;

             int rx = mx - grid_x - 20;
             int ry = my - grid_y - 20;
             if (rx < 0) rx = 0; 
             if (ry < 0) ry = 0; 
             
             int col = rx / item_w;
             int row = ry / item_h;
             int idx = row * cols + col;
             
             if (idx < fm_state.entry_count) {
                 if (fm_state.selected_index == idx) {
                      char path[256];
                      strcpy(path, fm_state.current_path);
                      if (path[strlen(path)-1] != '/') strcat(path, "/");
                      strcat(path, fm_state.entry_cache[idx].name);
                      
                      int is_file = 0;
                      if (fm_strchr(fm_state.entry_cache[idx].name, '.')) is_file = 1;
                      
                      if (is_file) {
                          extern void text_editor_open(const char* list);
                          text_editor_open(path);
                      } else {
                          file_manager_open_directory(path);
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
    }
    
    // Keyboard shortcuts
    if (event->type == GUI_EVENT_KEY_PRESS) {
        char key = event->keyboard.key;
        uint8_t modifiers = event->keyboard.modifiers;
        
        // Ctrl key combinations
        if (modifiers & 0x01) {  // Ctrl pressed
            switch (key) {
                case 'c':
                case 'C':
                    fm_action_copy();
                    break;
                case 'x':
                case 'X':
                    fm_action_cut();
                    break;
                case 'v':
                case 'V':
                    fm_action_paste();
                    break;
                case 'n':
                case 'N':
                    if (modifiers & 0x02) {  // Ctrl+Shift+N
                        fm_action_new_folder();
                    } else {  // Ctrl+N
                        fm_action_new_file();
                    }
                    break;
            }
        } else {
            // Non-Ctrl keys
            switch (key) {
                case 127:  // Delete key
                    fm_action_delete();
                    break;
                case '\n':  // Enter - open selected item
                case '\r':
                    if (fm_state.selected_index >= 0) {
                        fm_action_open(fm_state.selected_index);
                    }
                    break;
                // F2 for rename (scancode would be better, but using approximation)
                // In a real implementation, check raw_code for F2
            }
        }
    }
}

void file_manager_show(void) {
    serial_write("[FM] Show requested.\n");

    if (!fm_state.window) {
        serial_write("[FM] Creating Window.\n");
        fm_state.window = gui_create_window("File Manager", 100, 100, 800, 550);
        if (!fm_state.window) return;

        fm_state.panel = gui_create_panel(0, 0, 800, 550 - 30);
        if (!fm_state.panel) return;

        fm_state.panel->base.draw = fm_draw_content;
        fm_state.panel->base.event_handler = fm_handle_event;
        
        gui_window_add_tab(fm_state.window, "Explorer", (gui_element_t*)fm_state.panel);
        gui_add_element(gui_mgr.root, (gui_element_t*)fm_state.window);

        file_manager_init();
        fm_refresh();
        
        // Debug
        // console_write("[FM] Window Added to Root\n");
    }
    
    if (fm_state.window->base.bounds.x < 0) { 
        fm_state.window->base.bounds.x = 80;
        fm_state.window->base.bounds.y = 80;
    }
    
    gui_bring_to_front((gui_element_t*)fm_state.window);
    fm_state.window->base.flags &= ~GUI_FLAG_HIDDEN;
    gui_mgr.needs_redraw = 1;
}
