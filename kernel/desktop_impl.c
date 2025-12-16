#include "desktop.h"
#include "apps/file_manager/file_manager.h"
#include "graphics.h"
#include "gui.h"
#include "mouse.h"
#include "string.h"
#include "rtc.h"
// #include "window.h" // Assuming window management will be integrated later

// Colors
#define DESKTOP_BG_COLOR     0x008080 // Teal
#define TASKBAR_COLOR        0xC0C0C0 // Silver/Gray
#define START_BTN_COLOR      0x008000 // Green
#define START_BTN_TEXT_COLOR 0xFFFFFF // White

#include "apps/terminal/terminal.h"
#include "cursor_icon.h"
#include "ports.h"
#include "icons_data.h"
#include "apps/text_editor/text_editor.h"
#include "apps/settings/settings.h"
#include "app_icons.h" // Added by user

// Procedures
void shutdown_system(void);
void restart_system(void);
void logout_system(void);

// Forward declarations
void settings_show(void);
void desktop_root_handler(gui_element_t *element, gui_event_t *event);
void toggle_start_menu(void);
void draw_top_bar(rect_t clip);
void draw_dock(rect_t clip);

// Procedural Wallpaper to mimic "Abstract Paper Cut"
// Using large circles with gradients/alpha
#include "wallpaper_data.h"

void draw_wallpaper(rect_t clip_rect) {
    (void)clip_rect; 
    graphics_draw_image(0, 0, 1024, 768, wallpaper_data);
}


// Icon Selection State
static int selected_icon = -1; // -1: None, 0: Computer, 1: Documents, 2: Terminal

// Forward declarations
void desktop_root_handler(gui_element_t *element, gui_event_t *event);
void toggle_start_menu(void);
void draw_top_bar(rect_t clip);
void draw_dock(rect_t clip);


// Wrapper for GUI system to draw the desktop
// Wrapper for GUI system to draw the desktop
void desktop_draw_element(gui_renderer_t *renderer, gui_element_t *element) {
    (void)renderer;
    // 1. Draw Desktop Background (Wallpaper) on the DIRTY rect
    // gui_run sets graphics clip to dirty rect, so we can just draw normally?
    // desktop_draw uses 0,0 1024,768. 
    // Optimization: Draw wallpaper only?
    // desktop_draw() does: desktop_draw_rect(0,0,1024,768);
    // which calls draw_wallpaper.
    
    // We should respect the clip passed/set by manager.
    // If gui_mgr sets clip, graphics_draw_image will clip automatically.
    desktop_draw(); 
    
    // 2. CRITICAL: Draw Children (Windows). 
    // The previous implementation STOPPED here, meaning children were NOT iterated 
    // by gui_draw_recursive if it called this custom draw.
    // Wait, gui_draw_recursive logic:
    // element->draw(renderer, element);
    // IF default -> draws children.
    // IF custom -> MUST draw children or manage them?
    // Let's check gui.c logic in mind (assumed or verified).
    // Usually custom draw replaces EVERYTHING.
    // So we must iterate children here. (Standard composite pattern)
    
    // 2. Children (Windows) are drawn by the caller (gui_draw_recursive)
    // gui.c: gui_draw_recursive calls element->draw(renderer, element).
    // It THEN iterates element->children.
    // So we just need to draw the background (Wallpaper) here.
}
// FIX: In gui.c (I checked before), gui_draw_recursive calls element->draw.
// Then it iterates children.
// So we DO NOT need to iterate children here.
// BUT, the previous `desktop_draw_element` implementation was:
// void desktop_draw_element(...) { desktop_draw(); }
// This is correct IF gui_draw_recursive continues to children.
// If artifacts happen, it means desktop_draw is NOT clearing the dirty area properly
// OR the dirty area is calculated wrong.
// desktop_draw calls desktop_draw_rect(0,0,1024,768).
// desktop_draw_rect calls draw_wallpaper(dirty). 
// The bug: desktop_draw_rect logic might be optimized to check intersection?
// Let's look at desktop_draw_rect in this file.

void desktop_init(void) {
    // Initialize any desktop state here
    terminal_init();
    
    // Override Root Event Handler for Desktop Keyboard Navigation AND Drawing
    if (gui_mgr.root) {
        gui_mgr.root->event_handler = desktop_root_handler;
        gui_mgr.root->draw = desktop_draw_element;
    }
}

// ... toggle_start_menu ... (unchanged)

// Polling for clock updates
static int last_minute = -1;

// Desktop Root Handler for Keyboard Navigation and Mouse Clicks
// ...
void desktop_check_clock(void) {
    rtc_time_t t = rtc_read_time();
    if (t.minute != last_minute) {
        last_minute = t.minute;
        // Invalidate top bar area (right side primarily, but simpler to do whole bar or right half)
        // Bar is 0,0 1024,24. Clock is at right.
        gui_invalidate_rect((rect_t){0, 0, 1024, 24});
    }
}

// Desktop Root Handler for Keyboard Navigation and Mouse Clicks
void desktop_root_handler(gui_element_t *element, gui_event_t *event) {
    // Call default first (handles styling etc if any)
    gui_default_event_handler(element, event);
    
    if (event->type == GUI_EVENT_KEY_PRESS) {
        uint8_t key = (uint8_t)event->keyboard.key;
        
        // Navigation: Up/Down/Left/Right
        // Scancodes: Up 0x48, Down 0x50, Left 0x4B, Right 0x4D
        // Enter: 0x1C
        
        if (key == 0x50 || key == 0x4D) { // Down or Right -> Next
            selected_icon++;
            if (selected_icon > 4) selected_icon = 0;
            gui_mgr.needs_redraw = 1;
        }
        else if (key == 0x48 || key == 0x4B) { // Up or Left -> Prev
            selected_icon--;
            if (selected_icon < 0) selected_icon = 4;
            gui_mgr.needs_redraw = 1;
        }
        else if (key == 0x1C) { // Enter -> Launch
            if (selected_icon == 0) file_manager_show(); 
            else if (selected_icon == 1) file_manager_show();
            else if (selected_icon == 2) terminal_show();
            else if (selected_icon == 3) text_editor_show();
            else if (selected_icon == 4) settings_show();
            }
        }

    else if (event->type == GUI_EVENT_MOUSE_DOWN) {
        int x = event->mouse.pos.x;
        int y = event->mouse.pos.y;
        
        // 1. Check Top Bar (Apple Menu)
        if (y < 24) {
             if (x < 50) {
                 toggle_start_menu(); // Apple Menu
             }
             return;
        }

        // 2. Dock Icons (Center Bottom)
        // Constants matching draw_dock
        int num_icons = 5;
        int icon_size = 48;
        int pad = 16;
        int dock_w = num_icons * (icon_size + pad) + pad;
        int dock_h = 70;
        int dock_x = (gui_mgr.screen_width - dock_w) / 2;
        int dock_y = gui_mgr.screen_height - dock_h - 10;
        

        
        // Check if inside dock area slightly padded
        if (x >= dock_x && x <= dock_x + dock_w && y >= dock_y) {
            
            // Determine which icon
            // dock_x + pad + i*(size+pad)
            
            int relative_x = x - (dock_x + pad);
            if (relative_x < 0) return;
            
            int icon_index = relative_x / (icon_size + pad);
            int remainder = relative_x % (icon_size + pad);
            
            if (icon_index >= 0 && icon_index < 5 && remainder <= icon_size) {
                 // Valid Click on Icon i
                 selected_icon = icon_index;
                 
                 if (selected_icon == 0) {
                     file_manager_show();
                 }
                 else if (selected_icon == 1) {
                     file_manager_show();
                 }
                 else if (selected_icon == 2) {
                     terminal_show();
                 }
                 else if (selected_icon == 3) {
                     text_editor_show();
                 }
                 else if (selected_icon == 4) {
                     settings_show();
                 }
                 
                 gui_mgr.needs_redraw = 1;
                 return;
            }
        }

        
        // Click elsewhere -> Deselect
        if (selected_icon != -1) {
            selected_icon = -1;
            gui_mgr.needs_redraw = 1;
        }
    }
}

// Helper for the notch
// Helper for the notch - REMOVED

// ... (previous helper function if any)

// Custom OS Logo (Stylized 'M')
void draw_os_logo(int x, int y) {
    // Elegant Geometric Logo
    // Center circle
    // draw_circle_filled(x + 10, y + 10, 8, 0xFF333333);
    
    // Abstract shape: Two overlapping circles or a diamond
    // Diamond
    // (x+10, y) -> (x+20, y+10) -> (x+10, y+20) -> (x, y+10)
    int color = 0xFF000000;
    
    // Draw M shape using lines
    // Left vertical
    draw_line(x+4, y+4, x+4, y+16, color);
    draw_line(x+5, y+4, x+5, y+16, color); // Bold
    
    // Diagonals
    draw_line(x+4, y+4, x+10, y+10, color);
    draw_line(x+10, y+10, x+16, y+4, color);
    
    // Right vertical
    draw_line(x+16, y+4, x+16, y+16, color);
    draw_line(x+15, y+4, x+15, y+16, color); // Bold
}

void draw_top_bar(rect_t clip) {
    (void)clip;
    // Elegant Menu Bar: Height 24px, Translucent White
    rect_t bar = {0, 0, gui_mgr.screen_width, 24};
    draw_rect_filled(bar, 0xCCFFFFFF); // Higher opacity (80%) for elegance
    
    // Bottom border - faint
    draw_line(0, 24, gui_mgr.screen_width, 24, 0x40A0A0A0);
    
    // 1. OS Logo (Left)
    draw_os_logo(15, 4); // x=15, slightly lower y=4 for centering relative to text
    
    // 2. App Name (Current App) - Bold
    // Simulated Bold: Draw twice with 1px offset
    // Compact spacing: 6px instead of 8px

    // Apple Logo (Using text for now, or custom?) -> "Apple"
    // Let's use ASCII char for apple if we had one, but we don't.
    // Just draw text "Apple" or similar.
    // Actually, user said "desktpop, file, edit, etc...".
    
    int start_x = 20;
    int text_y = 5; // (24 - 16) / 2 = 4. 16px height.
    
    // Bold? We only have one weight.
    draw_text_sf("Mithl", start_x, text_y, 0xFF000000);
    start_x += get_text_width_sf("Mithl") + 20;
    
    draw_text_sf("File", start_x, text_y, 0xFF000000);
    start_x += get_text_width_sf("File") + 15;
    
    draw_text_sf("Edit", start_x, text_y, 0xFF000000);
    start_x += get_text_width_sf("Edit") + 15;
    
    draw_text_sf("View", start_x, text_y, 0xFF000000);
    start_x += get_text_width_sf("View") + 15;
    
    draw_text_sf("Go", start_x, text_y, 0xFF000000);
    start_x += get_text_width_sf("Go") + 15;
    
    draw_text_sf("Window", start_x, text_y, 0xFF000000);
    start_x += get_text_width_sf("Window") + 15;
    
    draw_text_sf("Help", start_x, text_y, 0xFF000000);
    
    // 5. Clock (Right)

    
    // RIGHT END: Date/Time (Digital Clock)
    rtc_time_t t = rtc_read_time();
    
    char time_str[32];
    int hour = t.hour;
    const char *ampm = "AM";
    if (hour >= 12) {
        ampm = "PM";
        if (hour > 12) hour -= 12;
    }
    if (hour == 0) hour = 12;
    
    int idx = 0;
    // Format: "Sun 12:45 PM" (Add Day?)
    
    if (hour < 10) { time_str[idx++] = '0' + hour; }
    else { time_str[idx++] = '0' + (hour/10); time_str[idx++] = '0' + (hour%10); }
    
    time_str[idx++] = ':';
    
    if (t.minute < 10) { time_str[idx++] = '0'; time_str[idx++] = '0' + t.minute; }
    else { time_str[idx++] = '0' + (t.minute/10); time_str[idx++] = '0' + (t.minute%10); }
    
    time_str[idx++] = ' ';
    time_str[idx++] = ampm[0];
    time_str[idx++] = ampm[1];
    time_str[idx] = '\0';
    
    // int clock_width = idx * text_spacing; 
    int clock_x = gui_mgr.screen_width - 100; // Fixed box for clock
    // Draw Time
    // Calculate width
    int w = get_text_width_sf(time_str);
    int x = clock_x + (80 - w) / 2; // Center in 80px box
    
    draw_text_sf(time_str, x, 5, 0xFF000000);
    
    // Status Icons (Left of Clock)
    
    // Search Icon (Magnifying Glass)
    // Clock X - 20 (gap) - Icon Width (e.g. 16)
    int search_x = clock_x - 30; // Gap
    int sy = 8;
    // Circle
    // draw_circle_stroke(search_x+6, sy+6, 5, 0xFF333333); // No circle func yet, simulate?
    // Rect simulate box? Let's use lines.
    // Glass
    draw_line(search_x+2, sy+2, search_x+8, sy+2, 0xFF333333);
    draw_line(search_x+8, sy+2, search_x+8, sy+8, 0xFF333333);
    draw_line(search_x+8, sy+8, search_x+2, sy+8, 0xFF333333);
    draw_line(search_x+2, sy+8, search_x+2, sy+2, 0xFF333333);
    // Handle
    draw_line(search_x+7, sy+7, search_x+11, sy+11, 0xFF333333);
    
    // WiFi
    int wifi_x = search_x - 30;
    // Arcs
    draw_line(wifi_x, 14, wifi_x+8, 14, 0xFF505050); // Dot
    draw_line(wifi_x-2, 11, wifi_x+10, 11, 0xFF505050); // Mid
    draw_line(wifi_x-4, 8, wifi_x+12, 8, 0xFF505050); // Top
    
    // Battery?
    // int bat_x = wifi_x - 30;
    // ...
}

void draw_dock(rect_t clip) {
    (void)clip; 
    
    // Floating Dock at bottom, wider for 5 icons
    int num_icons = 5;
    int icon_size = 48;
    int pad = 16;
    int dock_w = num_icons * (icon_size + pad) + pad;
    int dock_h = 70;
    int dock_x = (gui_mgr.screen_width - dock_w) / 2;
    int dock_y = gui_mgr.screen_height - dock_h - 10;
    
    // Color: 20-30% White
    uint32_t color = 0x40FFFFFF;
    int radius = 18;
    
    // 1. Draw Center Body (Rect)
    // From y + radius to y + h - radius
    rect_t center_rect = {dock_x, dock_y + radius, dock_w, dock_h - 2 * radius};
    draw_rect_filled(center_rect, color);
    
    // 2. Draw Top and Bottom Caps (Scanlines)
    for (int dy = 0; dy < radius; dy++) {
        // Distance from center of circle (which is at radius)
        // dy=0 is top edge (dist=radius). dy=radius-1 is near center (dist=1).
        // Actually, let's work from center out.
        // Circle center for TOP is at y_center = dock_y + radius.
        // Line y = dock_y + radius - dy - 1. (dy goes 0..radius-1)
        // dy is distance from center line up.
        
        int dist = radius - dy; // 18 down to 1
        // x offset = sqrt(r^2 - dist^2)
        // Simple integer sqrt approximation or loop match
        int half_chord = 0;
        // Find x where x*x + dist*dist <= r*r
        // We want max x.
        for (int x = 0; x <= radius; x++) {
             if (x*x + dist*dist <= radius*radius) half_chord = x;
             else break;
        }
        
        // Width of line at this y is (w - 2*r) + 2*half_chord
        // Start x is (dock_x + r) - half_chord
        
        int line_x = dock_x + radius - half_chord;
        int line_w = (dock_w - 2 * radius) + 2 * half_chord;
        
        // Top Line
        int top_y = dock_y + dy;
        draw_rect_filled((rect_t){line_x, top_y, line_w, 1}, color);
        
        // Bottom Line
        // Bottom center y = dock_y + dock_h - radius.
        // Line is y = center + dist.
        // wait, dy goes 0..17. 0 is top.
        // dist = radius - dy.
        // top_y is correct.
        
        int bottom_y = (dock_y + dock_h - 1) - dy;
        draw_rect_filled((rect_t){line_x, bottom_y, line_w, 1}, color);
    }
    
    // Draw Icons inside Dock
    // 0: Computer, 1: Folder, 2: Terminal, 3: Editor, 4: Settings
    // We reuse the global icon pointers
    
    int start_x = dock_x + pad;
    int yp = dock_y + (dock_h - icon_size) / 2;

    draw_icon(start_x, yp, icon_app_computer);
    draw_icon(start_x + 1*(icon_size+pad), yp, icon_app_filemanager);
    draw_icon(start_x + 2*(icon_size+pad), yp, icon_app_terminal);
    draw_icon(start_x + 3*(icon_size+pad), yp, icon_app_notepad);
    draw_icon(start_x + 4*(icon_size+pad), yp, icon_app_settings);
}

void draw_icons(void) {
    // 1. Computer (0)
    int y_start = 60; // Moved down due to Menu Bar
    int y_step = 90;
    
    // Draw "Computer" Icon
    draw_icon(28, y_start, icon_computer);
    draw_text("My Computer", 10, y_start + 55, 0xFFFFFF, 12);
    if (selected_icon == 0) {
        // Transparent White Border (RGBA: 50% White = 0x80FFFFFF)
        draw_round_rect((rect_t){25, y_start - 5, 70, 70}, 8, 0x80FFFFFF);
    }
    
    // 2. Documents (1)
    int y_doc = y_start + y_step;
    draw_icon(28, y_doc, icon_folder);
    draw_text("Documents", 15, y_doc + 55, 0xFFFFFF, 12);
    if (selected_icon == 1) {
        draw_round_rect((rect_t){25, y_doc - 5, 70, 70}, 8, 0x80FFFFFF);
    }
    
    // 3. Terminal (2)
    int y_term = y_start + y_step * 2;
    draw_icon(28, y_term, icon_terminal);
    draw_text("Terminal", 20, y_term + 55, 0xFFFFFF, 12);
    if (selected_icon == 2) {
        draw_round_rect((rect_t){25, y_term - 5, 70, 70}, 8, 0x80FFFFFF);
    }

    // 4. Text Editor (3)
    int y_edit = y_start + y_step * 3;
    draw_icon(28, y_edit, icon_text_editor);
    draw_text("Text Editor", 15, y_edit + 55, 0xFFFFFF, 12);
    if (selected_icon == 3) {
        draw_round_rect((rect_t){25, y_edit - 5, 70, 70}, 8, 0x80FFFFFF);
    }

    // 5. Settings (4)
    int y_set = y_start + y_step * 4;
    draw_icon(28, y_set, icon_settings);
    draw_text("Settings", 20, y_set + 55, 0xFFFFFF, 12);
    if (selected_icon == 4) {
        draw_round_rect((rect_t){25, y_set - 5, 70, 70}, 8, 0x80FFFFFF);
    }
}



// Start Menu Logic
static gui_window_t *start_menu = NULL;

// Click handlers for Start Menu items
// Click handlers for Start Menu items
void btn_about_click(gui_element_t *el, gui_event_t *ev) {
    (void)el; (void)ev;
    if (ev->type == GUI_EVENT_MOUSE_UP) {
        toggle_start_menu(); // Close menu
        settings_show(); // Open Settings (About This Mac)
    }
}
void btn_shutdown_click(gui_element_t *el, gui_event_t *ev) {
    (void)el;
    if (ev->type == GUI_EVENT_MOUSE_UP) shutdown_system();
}
void btn_restart_click(gui_element_t *el, gui_event_t *ev) {
    (void)el;
    if (ev->type == GUI_EVENT_MOUSE_UP) restart_system();
}

// Custom Draw for Apple Menu (Frameless Dropdown)
void draw_apple_menu(gui_renderer_t *renderer, gui_element_t *element) {
    (void)renderer;
    rect_t b = element->bounds;
    
    // 1. Shadow (Simple offset rect)
    // draw_rect_filled((rect_t){b.x + 4, b.y + 4, b.width, b.height}, 0x40000000); 
    
    // 2. White Background
    draw_rect_filled(b, 0xFFFFFF);
    
    // 3. Border (Thin Gray)
    draw_rect((rect_t){b.x, b.y, b.width, b.height}, 0xCCCCCC);
    
    // 4. Children (Labels) are drawn by recursive call in GUI manager?
    // Wait, gui_draw_recursive calls element->draw. If element->draw doesn't draw children, 
    // we must rely on gui_draw_recursive logic. 
    // gui_draw_recursive calls element->draw OR default, THEN iterates children.
    // So we just draw the background here.
}

void toggle_start_menu(void) {
    if (!start_menu) {
        // Create Apple Menu (Dropdown at Top-Left)
        // x=5, y=24 (Height of top bar)
        start_menu = gui_create_window("Start", 5, 24, 160, 100);
        if (start_menu) {
             start_menu->base.background_color = 0xFFFFFF; 
             
             // CUSTOMIZE for Menu Look
             start_menu->base.draw = draw_apple_menu;
             start_menu->base.event_handler = gui_default_event_handler; // No dragging
             
             gui_add_element(gui_mgr.root, (gui_element_t*)start_menu);
             gui_bring_to_front((gui_element_t*)start_menu);
             
             // Items
             // 1. About This Mac
             // Adjust Y to be relative to 0 since no title bar
             gui_label_t *l1 = gui_create_label("About This Mac", 10, 10, 140, 20);
             l1->base.event_handler = btn_about_click;
             l1->base.background_color = 0xFFFFFF; 
             gui_add_element((gui_element_t*)start_menu, (gui_element_t*)l1);
             
             // 2. Restart
             gui_label_t *l_restart = gui_create_label("Restart...", 10, 35, 140, 20);
             l_restart->base.event_handler = btn_restart_click;
             l_restart->base.background_color = 0xFFFFFF;
             gui_add_element((gui_element_t*)start_menu, (gui_element_t*)l_restart);

             // 3. Shut Down
             gui_label_t *l_shutdown = gui_create_label("Shut Down...", 10, 60, 140, 20);
             l_shutdown->base.event_handler = btn_shutdown_click;
             l_shutdown->base.background_color = 0xFFFFFF;
             gui_add_element((gui_element_t*)start_menu, (gui_element_t*)l_shutdown);
        }
    } else {
        if (start_menu->base.bounds.x == -1000) {
             start_menu->base.bounds.x = 5; // Show at Top-Left
             gui_mgr.focused_element = (gui_element_t*)start_menu;
             gui_bring_to_front((gui_element_t*)start_menu);
        } else {
             start_menu->base.bounds.x = -1000; // Hide
        }
        gui_mgr.needs_redraw = 1;
    }
}

void desktop_update(void) {
    // Logic moved to GUI system
    gui_run();
}

void draw_taskbar(void) {
    // Taskbar removed in favor of Dock + Top Bar
}



// desktop_draw_rect is called by desktop_draw -> desktop_draw_element (Root Draw)
void desktop_draw_rect(int x, int y, int w, int h) {
    (void)x; (void)y; (void)w; (void)h;
    
    // We rely on the graphics driver clipping set by gui_mgr before calling this.
    // So we just draw the FULL wallpaper. The driver clips it to the dirty rect.
    // This ensures the background is ALWAYS restored for the dirty area.
    
    graphics_draw_image(0, 0, 1024, 768, wallpaper_data);
    
    // 3. Top Bar Overlap
    // Always call, relies on clipping.
    draw_top_bar(gui_mgr.dirty_rect);
    
    // 4. Dock Overlap
    // Always call, relies on clipping.
    draw_dock(gui_mgr.dirty_rect);
    
    // 5. Icons
    // Moved to desktop_draw_element call or similar? 
    // Not seeing icons here? Ah, "Icons are now in Dock only" comment.
    // But verify: draw_dock handles icons.
}

// --- System Functions ---

void desktop_draw(void) {
    desktop_draw_rect(0, 0, 1024, 768);
}

void shutdown_system(void) {
    // Show Shutdown Screen
    draw_boot_logo(); 
    
    // Draw Overlay Text
    rect_t text_rect = {0, 650, 1024, 50};
    draw_text_centered("Shutting Down...", text_rect, 0xFFFFFF, 24);
    graphics_swap_buffers();
    
    for (volatile int i = 0; i < 200000000; i++);

    // ACPI Shutdown (VirtualBox/QEMU)
    outw(0xB004, 0x2000);
    outw(0x604, 0x2000);
    asm volatile("cli; hlt");
}

void restart_system(void) {
    draw_boot_logo();
    rect_t text_rect = {0, 650, 1024, 50};
    draw_text_centered("Restarting...", text_rect, 0xFFFFFF, 24);
    graphics_swap_buffers();
    
    for (volatile int i = 0; i < 200000000; i++);

    // Keyboard controller reset
    uint8_t temp;
    do {
        temp = inb(0x64);
        if (temp & 1) inb(0x60);
    } while (temp & 2);
    outb(0x64, 0xFE);
}

void logout_system(void) {
    desktop_init();
    gui_mgr.needs_redraw = 1;
}
