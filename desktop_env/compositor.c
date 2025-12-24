#include <desktop.h>
#include <apps/file_manager/file_manager.h>
#include <graphics.h>
#include <gui.h>
#include <mouse.h>
#include <string.h>
#include <rtc.h>

#include <apps/terminal/terminal.h>
#include <cursor_icon.h>
#include <ports.h>
#include <icons_data.h>
#include <apps/text_editor/text_editor.h>
#include <apps/settings/settings.h>
#include <app_icons.h> // Added by user
#include <console.h>
#include <bootlogo.h>
#include <memory.h>
#include <theme.h>

/* 
 * Mithl OS Compositor / Desktop Environment
 * Inspired by Wayland Compositors
 * 
 * Located in desktop_env/
 */

// Colors
#define DESKTOP_BG_COLOR     0x008080 // Teal
#define TASKBAR_COLOR        0xC0C0C0 // Silver/Gray
#define START_BTN_COLOR      0x008000 // Green
#define START_BTN_TEXT_COLOR 0xFFFFFF // White

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
static int get_dock_rect(rect_t *r);

// Procedural Wallpaper to mimic "Abstract Paper Cut"
// Using large circles with gradients/alpha
#include <wallpaper_data.h>

void draw_wallpaper(rect_t clip_rect) {
    (void)clip_rect; 
    graphics_draw_image(0, 0, 1024, 768, wallpaper_data);
}

// Icon Selection State
static int selected_icon = -1; // -1: None, 0: Computer, 1: Documents, 2: Terminal

// Wrapper for GUI system to draw the desktop
void desktop_draw_element(gui_renderer_t *renderer, gui_element_t *element) {
    (void)renderer;
    (void)element;
    
    // Optimize: Set clip rect to dirty area to prevent full wallpaper redraw
    graphics_set_clip(gui_mgr.dirty_rect);
    
    // 1. Draw Desktop Background (Wallpaper) on the DIRTY rect
    desktop_draw(); 
    
    // Reset clip to full screen for children/subsequent draws if needed
    // (Ideally GUI should manage this stack, but for now reset to safe default)
    graphics_set_clip((rect_t){0, 0, 1024, 768});
    
    // 2. Children (Windows) are drawn by the caller (gui_draw_recursive)
}

void desktop_init(void) {
    // 1. Init Theme (Services)
    theme_init();
    
    // Initialize any desktop state here
    // terminal_init(); // Removed to prevent auto-spawn
    
    // console_write("[COMPOSITOR] Auto-launching File Manager...\n");
    // file_manager_show();
    
    // Override Root Event Handler for Desktop Keyboard Navigation AND Drawing
    if (gui_mgr.root) {
        gui_mgr.root->event_handler = desktop_root_handler;
        gui_mgr.root->draw = desktop_draw_element;
    }
}

// Polling for clock updates
static int last_minute = -1;

void desktop_check_clock(void) {
    rtc_time_t t = rtc_read_time();
    if (t.minute != last_minute) {
        last_minute = t.minute;
        // Invalidate top bar area
        gui_invalidate_rect((rect_t){0, 0, 1024, 24});
    }
}


// Command Bar State
static int cmdbar_visible = 0;
static char cmdbar_buf[128];
static int cmdbar_len = 0;

void hide_command_bar() {
    cmdbar_visible = 0;
    cmdbar_len = 0;
    cmdbar_buf[0] = 0;
    gui_mgr.needs_redraw = 1;
}

void execute_command_bar() {
    if (cmdbar_len == 0) return;
    
    console_write("[CMDBAR] Executing: ");
    console_write(cmdbar_buf);
    console_write("\n");
    
    // 1. Semantic Query
    // We are in Kernel, so we call kernel functions directly
    extern int agent_query(const char *query, char *buffer, int size);
    extern void *process_create_elf(const char *name, const char *filename, const char *args);
    
    char binary[128];
    if (agent_query(cmdbar_buf, binary, 128) == 0) {
        // Found Agent
        // Need to resolve path? agent path is usually absolute or relative
        // process_create_elf expects full path or needs VFS lookup
        // We do naive check
        char path[128];
        strcpy(path, binary);
        
        // If not absolute, prepend /bin/ ?
        if (path[0] != '/') {
            // Try /bin/
            char temp[128];
            strcpy(temp, "/bin/");
            strcat(temp, path);
            strcpy(path, temp);
        }
        
        process_create_elf("Agent", path, "");
    } else {
        // No match, try running as direct command
         char path[128];
         strcpy(path, cmdbar_buf);
         if (path[0] != '/') {
            char temp[128];
            strcpy(temp, "/bin/");
            strcat(temp, path);
            strcpy(path, temp);
         }
         // Append .elf if missing
         int len = strlen(path);
         if (len < 4 || strcmp(path+len-4, ".elf") != 0) {
             strcat(path, ".elf");
         }
         
         process_create_elf("Command", path, "");
    }
    
    hide_command_bar();
}

// Desktop Root Handler for Keyboard Navigation and Mouse Clicks
void desktop_root_handler(gui_element_t *element, gui_event_t *event) {
    gui_default_event_handler(element, event);
    
    if (event->type == GUI_EVENT_KEY_PRESS) {
        uint8_t key = (uint8_t)event->keyboard.key;
        uint8_t mod = event->keyboard.modifiers;
        char ascii = event->keyboard.raw_code; // Wait, raw_code or ascii? gui_event translation in gui.c puts ASCII in .key if typable?
        // gui_event_t definition: union { char key; modifiers; raw_code; } keyboard
        // Usually .key is ASCII char if printable.
        
        // Command Bar Toggle
        // Allow META (Win) OR ALT + SPACE
        if (((mod & KEY_MOD_META) || (mod & KEY_MOD_ALT)) && key == ' ') {
            cmdbar_visible = !cmdbar_visible;
            if (cmdbar_visible) {
                cmdbar_len = 0; 
                cmdbar_buf[0] = 0;
            }
            gui_mgr.needs_redraw = 1;
            return;
        }
        
        // Command Bar Input
        if (cmdbar_visible) {
            if (key == '\n' || key == 0x1C) { // Enter
                execute_command_bar();
            } else if (key == 0x01 || key == 0x1B) { // ESC
                hide_command_bar();
            } else if (key == '\b') {
                if (cmdbar_len > 0) {
                    cmdbar_buf[--cmdbar_len] = 0;
                    gui_mgr.needs_redraw = 1;
                }
            } else if (key >= 32 && key < 127) {
                if (cmdbar_len < 64) {
                    cmdbar_buf[cmdbar_len++] = key;
                    cmdbar_buf[cmdbar_len] = 0;
                    gui_mgr.needs_redraw = 1;
                }
            }
            return; // Consume event if bar visible
        }

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
        
        // 1. Check Start Bubble (Bottom Left of Dock)
        rect_t dock;
        get_dock_rect(&dock);
        
        int bubble_w = 70;
        int bubble_h = 70;
        int bubble_x = dock.x - bubble_w - 15;
        int bubble_y = dock.y;
        
        if (x >= bubble_x && x <= bubble_x + bubble_w && y >= bubble_y && y <= bubble_y + bubble_h) {
             toggle_start_menu();
             return;
        }

        /* Top Bar Removed
        if (y < 24) {
             if (x < 50) {
                 toggle_start_menu(); // Apple Menu
             }
             return;
        }
        */

        // 2. Dock Icons (Center Bottom)
        // Constants matching draw_dock
        int num_icons = 6;
        int icon_size = 48;
        int pad = 16;
        int dock_w = num_icons * (icon_size + pad) + pad;
        int dock_h = 70;
        int dock_x = (gui_mgr.screen_width - dock_w) / 2;
        int dock_y = gui_mgr.screen_height - dock_h - 10;
        
        // Check if inside dock area slightly padded
        if (x >= dock_x && x <= dock_x + dock_w && y >= dock_y) {
            int relative_x = x - (dock_x + pad);
            if (relative_x < 0) return;
            
            int icon_index = relative_x / (icon_size + pad);
            int remainder = relative_x % (icon_size + pad);
            
            if (icon_index >= 0 && icon_index < num_icons && remainder <= icon_size) {
                 // Valid Click on Icon i
                 selected_icon = icon_index;
                 
                 // DEBUG: Print which icon was clicked
                 console_write("[COMPOSITOR] Icon clicked: ");
                 if (icon_index == 0) console_write("0 (Computer/FM)\n");
                 else if (icon_index == 1) console_write("1 (File Manager)\n");
                 else if (icon_index == 2) console_write("2 (Terminal)\n");
                 else if (icon_index == 3) console_write("3 (Notepad)\n");
                 else if (icon_index == 4) console_write("4 (Settings)\n");
                 else if (icon_index == 5) console_write("5 (Doom)\n");
                 
                 if (selected_icon == 0) {
                     console_write("[COMPOSITOR] Launching File Manager (icon 0)\n");
                     file_manager_show();
                 }
                 else if (selected_icon == 1) {
                     serial_write("[COMPOSITOR] Launching File Manager (icon 1)\n");
                     file_manager_show();
                 }
                 else if (selected_icon == 5) {
                     // Doom - call the launch function from kernel.c
                     extern void launch_doom_safe(void);
                     console_write("[COMPOSITOR] Launching Doom via kernel launcher\n");
                     launch_doom_safe();
                 }
                 else if (selected_icon == 2) {
                     console_write("[COMPOSITOR] Launching Terminal\n");
                     terminal_show();
                 }
                 else if (selected_icon == 3) {
                     console_write("[COMPOSITOR] Launching Text Editor\n");
                     text_editor_show();
                 }
                 else if (selected_icon == 4) {
                     console_write("[COMPOSITOR] Launching Settings\n");
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

// Custom OS Logo
void draw_os_logo(int x, int y) {
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
    int start_x = 20;
    int text_y = 5; // (24 - 16) / 2 = 4. 16px height.
    
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
    
    if (hour < 10) { time_str[idx++] = '0' + hour; }
    else { time_str[idx++] = '0' + (hour/10); time_str[idx++] = '0' + (hour%10); }
    
    time_str[idx++] = ':';
    
    if (t.minute < 10) { time_str[idx++] = '0'; time_str[idx++] = '0' + t.minute; }
    else { time_str[idx++] = '0' + (t.minute/10); time_str[idx++] = '0' + (t.minute%10); }
    
    time_str[idx++] = ' ';
    time_str[idx++] = ampm[0];
    time_str[idx++] = ampm[1];
    time_str[idx] = '\0';
    
    int clock_x = gui_mgr.screen_width - 100; // Fixed box for clock
    // Calculate width
    int w = get_text_width_sf(time_str);
    int x = clock_x + (80 - w) / 2; // Center in 80px box
    
    draw_text_sf(time_str, x, 5, 0xFF000000);
    
    // Status Icons (Left of Clock)
    // Search Icon (Magnifying Glass)
    int search_x = clock_x - 30; // Gap
    int sy = 8;
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
}

// desktop_draw_rect moved to bottom


void draw_dock(rect_t clip) {
    (void)clip; 
    
    // Floating Dock at bottom, wider for 6 icons (Doom added)
    int num_icons = 6;
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
    rect_t center_rect = {dock_x, dock_y + radius, dock_w, dock_h - 2 * radius};
    draw_rect_filled(center_rect, color);
    
    // 2. Draw Top and Bottom Caps (Scanlines)
    for (int dy = 0; dy < radius; dy++) {
        int dist = radius - dy; 
        int half_chord = 0;
        for (int x = 0; x <= radius; x++) {
             if (x*x + dist*dist <= radius*radius) half_chord = x;
             else break;
        }
        
        int line_x = dock_x + radius - half_chord;
        int line_w = (dock_w - 2 * radius) + 2 * half_chord;
        
        // Top Line
        int top_y = dock_y + dy;
        draw_rect_filled((rect_t){line_x, top_y, line_w, 1}, color);
        
        // Bottom Line
        int bottom_y = (dock_y + dock_h - 1) - dy;
        draw_rect_filled((rect_t){line_x, bottom_y, line_w, 1}, color);
    }
    
    // Draw Icons inside Dock
    int start_x = dock_x + pad;
    int yp = dock_y + (dock_h - icon_size) / 2;

    draw_icon(start_x, yp, icon_app_computer);
    draw_icon(start_x + 1*(icon_size+pad), yp, icon_app_filemanager);
    draw_icon(start_x + 2*(icon_size+pad), yp, icon_app_terminal);
    draw_icon(start_x + 3*(icon_size+pad), yp, icon_app_notepad);
    draw_icon(start_x + 4*(icon_size+pad), yp, icon_app_settings);
    // Doom Icon (Reusing Terminal icon for now, user requested icon)
    draw_icon(start_x + 5*(icon_size+pad), yp, icon_app_doom); 
}

void draw_icons(void) {
    // Icons logic removed or integrated into Dock
}

// Start Menu Logic
static gui_window_t *start_menu = NULL;

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
    
    // 2. White Background
    draw_rect_filled(b, 0xFFFFFF);
    
    // 3. Border (Thin Gray)
    draw_rect((rect_t){b.x, b.y, b.width, b.height}, 0xCCCCCC);
}

void toggle_start_menu(void) {
    // Calculate bubble position again for alignment
    rect_t dock;
    get_dock_rect(&dock);
    int bubble_w = 70;
    int bubble_x = dock.x - bubble_w - 15;
    int bubble_y = dock.y;
    
    // Y = Bubble Y - Menu Height - Gap
    int menu_h = 100;
    int menu_y = bubble_y - menu_h - 10;

    if (!start_menu) {
        // Create Start Menu (Popup near bubble)
        
        start_menu = gui_create_window("Start", bubble_x, menu_y, 160, menu_h);
        if (start_menu) {
             start_menu->base.background_color = 0xFFFFFF; 
             
             // CUSTOMIZE for Menu Look
             start_menu->base.draw = draw_apple_menu;
             start_menu->base.event_handler = gui_default_event_handler; 
             
             gui_add_element(gui_mgr.root, (gui_element_t*)start_menu);
             gui_bring_to_front((gui_element_t*)start_menu);
             
             // Items
             // 1. About
             gui_label_t *l1 = gui_create_label("About Mithl OS", 10, 10, 140, 20);
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
             start_menu->base.bounds.x = bubble_x; // Show
             start_menu->base.bounds.y = menu_y;   // Enforce Y
             gui_mgr.focused_element = (gui_element_t*)start_menu;
             gui_bring_to_front((gui_element_t*)start_menu);
             serial_write("[GUI] Start Menu Opened.\n");
        } else {
             start_menu->base.bounds.x = -1000; // Hide
             serial_write("[GUI] Start Menu Closed.\n");
        }
        gui_mgr.needs_redraw = 1;
    }
}

void desktop_update(void) {
    gui_run();
}

void draw_taskbar(void) {
    // Taskbar removed in favor of Dock + Top Bar
}

// --- Bubble UI Constants ---
// Dock is centered. Bubbles are to the left and right.
static int get_dock_rect(rect_t *r) {
    int num_icons = 6;
    int icon_size = 48;
    int pad = 16;
    int dock_w = num_icons * (icon_size + pad) + pad;
    int dock_h = 70;
    int dock_x = (gui_mgr.screen_width - dock_w) / 2;
    int dock_y = gui_mgr.screen_height - dock_h - 10;
    
    if (r) *r = (rect_t){dock_x, dock_y, dock_w, dock_h};
    return dock_x;
}

// Helper to get average luminance of wallpaper under a rect
static int get_average_luminance(rect_t r) {
    // wallpaper_data is a static array, address is always non-null

    
    int screen_w = gui_mgr.screen_width;
    int screen_h = gui_mgr.screen_height;

    // Clamp to screen bounds
    if (r.x < 0) r.x = 0;
    if (r.y < 0) r.y = 0;
    if (r.x + r.width > screen_w) r.width = screen_w - r.x;
    if (r.y + r.height > screen_h) r.height = screen_h - r.y;

    uint64_t total_lum = 0;
    int count = 0;
    
    // Sample every few pixels for performance
    for (int y = r.y; y < r.y + r.height; y += 4) {
        for (int x = r.x; x < r.x + r.width; x += 4) {
            uint32_t color = wallpaper_data[y * screen_w + x];
            // Format is usually ARGB or XRGB.
            uint8_t r_val = (color >> 16) & 0xFF;
            uint8_t g_val = (color >> 8) & 0xFF;
            uint8_t b_val = (color) & 0xFF; // Only B
            // Perceived luminance formula
            total_lum += (r_val * 299 + g_val * 587 + b_val * 114) / 1000;
            count++;
        }
    }
    
    if (count == 0) return 0;
    return total_lum / count;
}

// Helper to get pixel from bootlogo with safety checks
static uint32_t get_logo_pixel(int x, int y) {
    if (x < 0) x = 0; 
    if (y < 0) y = 0;
    if (x >= bootlogo_width) x = bootlogo_width - 1;
    if (y >= bootlogo_height) y = bootlogo_height - 1;
    return bootlogo_data[y * bootlogo_width + x];
}

static void draw_scaled_os_logo(int x, int y, int w, int h) {
    int src_w = bootlogo_width;
    int src_h = bootlogo_height;
    
    // Target size ~64 width to fill bubble nicely
    int target_w = 64; 
    int target_h = (src_h * target_w) / src_w;
    
    int offset_x = x + (w - target_w) / 2;
    int offset_y = y + (h - target_h) / 2;

    // Fixed point 16.16
    #define FP_SHIFT 16
    
    // Calculate steps
    int step_x = (src_w << FP_SHIFT) / target_w;
    int step_y = (src_h << FP_SHIFT) / target_h;
    
    // Allocate temp buffer for the rendered logo
    uint32_t buffer_size = target_w * target_h * sizeof(uint32_t);
    uint32_t *logo_buf = (uint32_t*)memory_alloc(buffer_size);
    
    if (!logo_buf) return;
    
    // 1. Render Bilinear Logo to Buffer
    for (int dy = 0; dy < target_h; dy++) {
        int v_fp = dy * step_y;
        for (int dx = 0; dx < target_w; dx++) {
            int u_fp = dx * step_x;
            
            // Coordinates in source (integer part)
            int src_x = u_fp >> FP_SHIFT;
            int src_y = v_fp >> FP_SHIFT;
            
            // Weights
            int w_x = u_fp & 0xFFFF;
            int w_y = v_fp & 0xFFFF;
            
            // Neighbor Check
            // Safety against OOB access
            uint32_t c00 = get_logo_pixel(src_x, src_y);
            uint32_t c10 = get_logo_pixel(src_x + 1, src_y);
            uint32_t c01 = get_logo_pixel(src_x, src_y + 1);
            uint32_t c11 = get_logo_pixel(src_x + 1, src_y + 1);
            
            // Bilinear Interp
            int a00 = (c00 >> 24) & 0xFF; int r00 = (c00 >> 16) & 0xFF; int g00 = (c00 >> 8) & 0xFF; int b00 = c00 & 0xFF;
            int a10 = (c10 >> 24) & 0xFF; int r10 = (c10 >> 16) & 0xFF; int g10 = (c10 >> 8) & 0xFF; int b10 = c10 & 0xFF;
            int a01 = (c01 >> 24) & 0xFF; int r01 = (c01 >> 16) & 0xFF; int g01 = (c01 >> 8) & 0xFF; int b01 = c01 & 0xFF;
            int a11 = (c11 >> 24) & 0xFF; int r11 = (c11 >> 16) & 0xFF; int g11 = (c11 >> 8) & 0xFF; int b11 = c11 & 0xFF;
            
            int inv_w_x = 65536 - w_x;
            int a0 = (a00 * inv_w_x + a10 * w_x) >> 16;
            int r0 = (r00 * inv_w_x + r10 * w_x) >> 16;
            int g0 = (g00 * inv_w_x + g10 * w_x) >> 16;
            int b0 = (b00 * inv_w_x + b10 * w_x) >> 16;
            
            int a1 = (a01 * inv_w_x + a11 * w_x) >> 16;
            int r1 = (r01 * inv_w_x + r11 * w_x) >> 16;
            int g1 = (g01 * inv_w_x + g11 * w_x) >> 16;
            int b1 = (b01 * inv_w_x + b11 * w_x) >> 16;
            
            int inv_w_y = 65536 - w_y;
            int a = (a0 * inv_w_y + a1 * w_y) >> 16;
            int r = (r0 * inv_w_y + r1 * w_y) >> 16;
            int g = (g0 * inv_w_y + g1 * w_y) >> 16;
            int b = (b0 * inv_w_y + b1 * w_y) >> 16;
            
            logo_buf[dy * target_w + dx] = (a << 24) | (r << 16) | (g << 8) | b;
        }
    }
    
    // 2. Draw Outline (Black shadow) - 8-neighbor for visibility
    for (int dy = 0; dy < target_h; dy++) {
        for (int dx = 0; dx < target_w; dx++) {
            uint32_t col = logo_buf[dy * target_w + dx];
            if (((col >> 24) & 0xFF) > 20) { // Threshold
                uint32_t border_col = 0xFF000000;
                
                // Cardinals
                set_pixel(offset_x + dx - 1, offset_y + dy, border_col);
                set_pixel(offset_x + dx + 1, offset_y + dy, border_col);
                set_pixel(offset_x + dx, offset_y + dy - 1, border_col);
                set_pixel(offset_x + dx, offset_y + dy + 1, border_col);
                
                // Diagonals
                set_pixel(offset_x + dx - 1, offset_y + dy - 1, border_col);
                set_pixel(offset_x + dx + 1, offset_y + dy - 1, border_col);
                set_pixel(offset_x + dx - 1, offset_y + dy + 1, border_col);
                set_pixel(offset_x + dx + 1, offset_y + dy + 1, border_col);
            }
        }
    }
    
    // 3. Draw Actual Logo
    for (int dy = 0; dy < target_h; dy++) {
        for (int dx = 0; dx < target_w; dx++) {
            uint32_t col = logo_buf[dy * target_w + dx];
            if (((col >> 24) & 0xFF) > 0) {
                set_pixel(offset_x + dx, offset_y + dy, col);
            }
        }
    }
    
    memory_free(logo_buf);
}

// Redefine to match prototype used in desktop_draw_rect
static void draw_start_bubble(void) {
    rect_t dock;
    get_dock_rect(&dock);
    
    int bubble_w = 70;
    int bubble_h = dock.height;
    int bubble_x = dock.x - bubble_w - 15;
    int bubble_y = dock.y;
    int radius = 16;
    
    theme_t *th = theme_get_current();

    rect_t bubble_rect = {bubble_x, bubble_y, bubble_w, bubble_h};

    // Draw Bubble Background
    draw_rounded_rect_filled(bubble_rect, radius, th->bubble_bg); 
    
    // Draw scaled OS Logo with outline
    draw_scaled_os_logo(bubble_x, bubble_y, bubble_w, bubble_h);
}

static void draw_status_bubble(void) {
    rect_t dock;
    get_dock_rect(&dock);

    int bubble_w = 200; // Wider
    int bubble_h = dock.height;
    int bubble_x = dock.x + dock.width + 15;
    int bubble_y = dock.y;
    int radius = 16; // Match dock corners
    
    theme_t *th = theme_get_current();

    rect_t bubble_rect = {bubble_x, bubble_y, bubble_w, bubble_h};

    // Draw Bubble Background
    draw_rounded_rect_filled(bubble_rect, radius, th->bubble_bg);

    // Get time & date
    rtc_time_t t = rtc_read_time(); 
    
    // --- TIME STRING ---
    char time_str[32];
    int hour = t.hour;
    char *ampm = "AM";
    if (hour >= 12) {
        ampm = "PM"; 
        if (hour > 12) hour -= 12;
    }
    if (hour == 0) hour = 12;
    
    int idx = 0;
    if (hour >= 10) { time_str[idx++] = '0' + (hour/10); time_str[idx++] = '0' + (hour%10); } 
    else { time_str[idx++] = '0' + hour; }
    
    time_str[idx++] = ':';
    if (t.minute < 10) { time_str[idx++] = '0'; }
    time_str[idx++] = '0' + (t.minute/10);
    time_str[idx++] = '0' + (t.minute%10);
    
    time_str[idx++] = ' ';
    time_str[idx++] = ampm[0];
    time_str[idx++] = ampm[1];
    time_str[idx] = 0;

    // --- DATE STRING (Format: DD/MM/YYYY) ---
    char date_str[32];
    idx = 0;
    if (t.day < 10) { date_str[idx++] = '0'; }
    if (t.day >= 10) { date_str[idx++] = '0' + (t.day/10); }
    date_str[idx++] = '0' + (t.day%10);
    
    date_str[idx++] = '/';
    
    if (t.month < 10) { date_str[idx++] = '0'; }
    if (t.month >= 10) { date_str[idx++] = '0' + (t.month/10); }
    date_str[idx++] = '0' + (t.month%10);
    
    date_str[idx++] = '/';
    
    // Year (2025 -> 20 25)
    int yr = t.year;
    if (yr < 2000) yr += 2000; // Guess century if needed, but usually full year
    
    int y1 = yr / 1000; int y2 = (yr/100)%10; int y3 = (yr/10)%10; int y4 = yr%10;
    date_str[idx++] = '0' + y1;
    date_str[idx++] = '0' + y2;
    date_str[idx++] = '0' + y3;
    date_str[idx++] = '0' + y4;
    date_str[idx] = 0;

    // Adaptive Text Color
    // Check luminance of the wallpaper area under the bubble
    // NOTE: This assumes transparency draws wallpaper. 
    // Since draw_rounded_rect_filled supports alpha blending if implemented, 
    // the text needs to stand out against the BLENDED result.
    int avg_lum = get_average_luminance(bubble_rect);
    uint32_t text_color = (avg_lum > 128) ? 0xFF000000 : 0xFFFFFFFF;
    
    // Layout: Time top, Date bottom
    int text_h = 16; // SF font height
    int gap = 2;     // Space between lines
    int total_h = text_h * 2 + gap;
    int start_y = bubble_y + (bubble_h - total_h) / 2;
    
    // Draw Time
    int time_w = get_text_width_sf(time_str);
    int time_x = bubble_x + (bubble_w - time_w) / 2;
    draw_text_sf(time_str, time_x, start_y, text_color);
    
    // Draw Date
    int date_w = get_text_width_sf(date_str);
    int date_x = bubble_x + (bubble_w - date_w) / 2;
    draw_text_sf(date_str, date_x, start_y + text_h + gap, text_color);

}

// desktop_draw_rect is called by desktop_draw -> desktop_draw_element (Root Draw)
void desktop_draw_rect(int x, int y, int w, int h) {
    (void)x; (void)y; (void)w; (void)h;
    
    // FIX: Clear buffer with solid color first to prevent transparency artifacts ("Hall of Mirrors")
    // When windows move, the old position needs to be erased. Since wallpaper might have transparency
    // or the previous buffer contents are preserved, we must clear it.
    draw_rect_filled((rect_t){0, 0, 1024, 768}, 0xFF000000);
    
    graphics_draw_image(0, 0, 1024, 768, wallpaper_data);
    
    // 4. Dock Overlap
    draw_dock(gui_mgr.dirty_rect);
    
    // 5. Bubbles
    draw_start_bubble();
    draw_status_bubble();
}

// Command Bar Draw
void draw_command_bar() {
    if (!cmdbar_visible) return;
    
    int w = 500;
    int h = 60;
    int x = (gui_mgr.screen_width - w) / 2;
    int y = gui_mgr.screen_height / 3; // Top-ish center
    
    // Glass Background
    rect_t bar_rect = {x, y, w, h};
    draw_rect_filled(bar_rect, 0xDD202020); // Dark translucent
    
    // Border
    draw_rect(bar_rect, 0xFF404040);
    
    // Text
    int text_x = x + 20;
    int text_y = y + (h - 16) / 2;
    
    if (cmdbar_len == 0) {
        draw_text_sf("Ask Mithl AI...", text_x, text_y, 0xFF808080);
    } else {
        draw_text_sf(cmdbar_buf, text_x, text_y, 0xFFFFFFFF);
        // Cursor
        draw_rect_filled((rect_t){text_x + get_text_width_sf(cmdbar_buf), text_y, 2, 16}, 0xFFFFFFFF);
    }
}

// --- System Functions ---

void desktop_draw(void) {
    desktop_draw_rect(0, 0, 1024, 768);
    draw_command_bar();
}

// External assembly functions
extern void asm_shutdown(void);
extern void asm_reboot(void);
// External ACPI
#include <acpi.h>

void shutdown_system(void) {
    // Clear screen and show shutdown message
    draw_boot_logo();
    
    // Show "Shutting down..." message
    rect_t text_rect = {0, 650, 1024, 50};
    draw_text_centered("Shutting down...", text_rect, 0xFFFFFFFF, 24);
    
    // Swap buffers to show message
    graphics_swap_buffers();
    
    // Small delay to show message
    for (volatile int i = 0; i < 50000000; i++);
    
    // 1. Try ACPI Shutdown (Standard)
    acpi_shutdown();
    
    // 2. Try Assembly Magic Ports (Fallback/VMs)
    asm_shutdown();
    
    // Fallback if asm fails
    while(1) {
        __asm__ volatile("hlt");
    }
}

void restart_system(void) {
    draw_boot_logo();
    rect_t text_rect = {0, 650, 1024, 50};
    draw_text_centered("Restarting...", text_rect, 0xFFFFFFFF, 24);
    graphics_swap_buffers();
    
    // Small delay
    for (volatile int i = 0; i < 50000000; i++);
    
    // Call Assembly Implementation
    asm_reboot();
    
    // Loop forever (should never reach here)
    while(1) {
        __asm__ volatile("hlt");
    }
}

void logout_system(void) {
    desktop_init();
    gui_mgr.needs_redraw = 1;
}
