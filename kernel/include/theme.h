#ifndef THEME_H
#define THEME_H

#include <stdint.h>

typedef struct {
    char name[32];
    
    // Window Colors
    uint32_t window_bg;           // Background of the window content
    uint32_t window_title_active; // Title bar color (active)
    uint32_t window_title_inactive;// Title bar color (inactive)
    uint32_t window_border;       // Border color
    uint32_t window_text;         // Default text color inside windows
    
    // Desktop Colors
    uint32_t desktop_bg;          // Solid color background (if no wallpaper)
    uint32_t taskbar_bg;          // Taskbar/Dock background
    uint32_t start_btn;           // Start button color
    uint32_t bubble_bg;           // Background for bubbles (Start/Time) usually with alpha
    
    // Accents
    uint32_t accent_color;        // Generic accent (blue, green, etc)
    uint32_t selection_bg;        // Text selection background
    
    // Terminal Specific
    uint32_t terminal_bg;
    uint32_t terminal_fg;
    
} theme_t;

// Global Theme Access
void theme_init();
theme_t* theme_get_current();
void theme_set_preset(int id); // 0=Dark, 1=Light

// Predefined Theme IDs
#define THEME_DARK  0
#define THEME_LIGHT 1

#endif
