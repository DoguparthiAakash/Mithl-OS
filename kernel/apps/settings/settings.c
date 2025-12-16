#include "settings.h"
#include "gui.h"
#include "graphics.h"
#include "memory.h"
#include "string.h"

// Settings App State
static gui_window_t *settings_win = NULL;
static gui_panel_t *settings_panel = NULL;
static int current_tab = 0; // 0: General, 1: Display, 2: Sound

// --- Event Handlers ---

static void settings_draw_content(gui_renderer_t *renderer, gui_element_t *element) {
    // 1. Draw Window Frame
    // gui_draw_window(renderer, element); // REMOVED
    draw_rect_filled(element->bounds, 0xFFFFFF);
    
    rect_t bounds = element->bounds;
    int title_h = 0; // No title bar internal
    int content_y = bounds.y + title_h;
    int sidebar_w = 150;
    
    // 2. Draw Sidebar Background (Light Gray)
    rect_t sidebar = {bounds.x, content_y, sidebar_w, bounds.height}; // Full height
    // Actually window draw handles main bg. We just overlay sidebar.
    // Sidebar usually goes all the way down.
    draw_rect_filled(sidebar, 0xF5F5F5);
    
    // Separator Line
    draw_line(bounds.x + sidebar_w, content_y, bounds.x + sidebar_w, bounds.y + bounds.height - 8, 0xD0D0D0);
    
    // 3. Draw Sidebar Items
    int item_h = 30;
    int start_y = content_y + 10;
    
    const char *items[] = {"General", "Display", "Sound", "Network", "Users"};
    int count = 5;
    
    for (int i = 0; i < count; i++) {
        int y = start_y + i * item_h;
        uint32_t text_col = 0x000000;
        
        // Highlight selection
        if (i == current_tab) {
            draw_rect_filled((rect_t){bounds.x + 10, y, sidebar_w - 20, 24}, 0x007AFF); // Blue Highlight
            text_col = 0xFFFFFF;
        }
        
        draw_text(items[i], bounds.x + 20, y + 6, text_col, 12);
    }
    
    // 4. Draw Main Content Area
    int main_x = bounds.x + sidebar_w + 20;
    int main_y = content_y + 20;
    
    if (current_tab == 0) {
        // General: About
        draw_text("About This Mac", main_x, main_y, 0x000000, 14); // Larger font if supported? No, scaled manually or just bold
        
        // Mock Icon for OS
        draw_circle_filled(main_x + 30, main_y + 50, 30, 0x008080); // Teal circle
        draw_text("M", main_x + 22, main_y + 42, 0xFFFFFF, 20); // Big M
        
        int info_x = main_x + 80;
        draw_text("Mithl OS", info_x, main_y + 30, 0x000000, 0);
        draw_text("Version 1.0 (Beta)", info_x, main_y + 50, 0x606060, 0);
        
        draw_text("Processor: Virtual CPU", info_x, main_y + 80, 0x404040, 0);
        draw_text("Memory: 128 MB", info_x, main_y + 100, 0x404040, 0);
        draw_text("Graphics: VESA 1024x768", info_x, main_y + 120, 0x404040, 0);
        
    } else if (current_tab == 1) {
        // Display
        draw_text("Display Settings", main_x, main_y, 0x000000, 0);
        draw_text("Resolution: 1024 x 768", main_x, main_y + 30, 0x404040, 0);
        draw_text("Brightness:", main_x, main_y + 60, 0x404040, 0);
        
        // Slider track
        draw_rect_filled((rect_t){main_x, main_y + 80, 200, 4}, 0xCCCCCC);
        // Slider knob
        draw_circle_filled(main_x + 150, main_y + 82, 8, 0xFFFFFF); // Knob
        // Knob border
        // draw_circle(main_x + 150, main_y + 82, 8, 0xAAAAAA);
        
    } else {
        draw_text("Not Implemented", main_x, main_y, 0x808080, 0);
    }
}

static void settings_handle_event(gui_element_t *element, gui_event_t *event) {
    // Base window handling
    // gui_window_event_handler(element, event); // REMOVED
    if (settings_win && settings_win->base.bounds.x == -9999) return;
    if (element->bounds.x == -9999) return;
    
    if (event->type == GUI_EVENT_MOUSE_DOWN) {
        int mx = event->mouse.pos.x;
        int my = event->mouse.pos.y;
        
        rect_t b = element->bounds;
        int sidebar_w = 150;
        int title_h = 24;
        
        // Check Sidebar Click
        if (mx >= b.x && mx < b.x + sidebar_w && my >= b.y + title_h) {
            int rel_y = my - (b.y + title_h + 10);
            if (rel_y >= 0) {
                int clicked_tab = rel_y / 30; // item_h
                if (clicked_tab >= 0 && clicked_tab < 5) {
                    current_tab = clicked_tab;
                    gui_mgr.needs_redraw = 1;
                }
            }
        }
    }
}

// --- Public API ---

void settings_show(void) {
    if (!settings_win) {
        // Create 600x400 window centered
        settings_win = gui_create_window("System Settings", 212, 184, 600, 400); 
        if (settings_win) {
             // Panel Logic
             settings_panel = gui_create_panel(0, 0, 600, 400 - 48);
             if (!settings_panel) return;
             
             settings_panel->base.draw = settings_draw_content;
             settings_panel->base.event_handler = settings_handle_event;
             settings_panel->base.background_color = 0xFFFFFF;
             
             gui_window_add_tab(settings_win, "General", (gui_element_t*)settings_panel);
        }
    }
    
    if (settings_win) {
        // Show if hidden
        if (settings_win->base.bounds.x == -9999) {
            settings_win->base.bounds.x = 212;
            settings_win->base.bounds.y = 184;
        }
        gui_mgr.focused_element = (gui_element_t*)settings_win;
        gui_mgr.needs_redraw = 1; 
    }
}
