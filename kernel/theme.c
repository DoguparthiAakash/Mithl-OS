#include <theme.h>
#include <string.h>

static theme_t current_theme;

void theme_init() {
    // Default to Dark Mode
    theme_set_preset(THEME_DARK);
}

theme_t* theme_get_current() {
    return &current_theme;
}

void theme_set_preset(int id) {
    if (id == THEME_LIGHT) {
        strcpy(current_theme.name, "Mithl Light");
        
        current_theme.window_bg           = 0xFFFFFFFF; // White
        current_theme.window_title_active = 0xFFCCCCCC; // Light Grey
        current_theme.window_title_inactive=0xFFE0E0E0;
        current_theme.window_border       = 0xFFAAAAAA;
        current_theme.window_text         = 0xFF000000; // Black
        
        current_theme.desktop_bg          = 0xFF3D6E70; // Muted Teal
        current_theme.taskbar_bg          = 0x40FFFFFF; // Glass
        current_theme.start_btn           = 0xFF333333; // Dark Grey Icon
        current_theme.bubble_bg           = 0x40FFFFFF; // Glass
        
        current_theme.accent_color        = 0xFF007ACC; // VS Code Blue
        current_theme.selection_bg        = 0xFFADD6FF;
        
        current_theme.terminal_bg         = 0xFFF0F0F0; // Very light grey
        current_theme.terminal_fg         = 0xFF222222; // Almost black
        
    } else {
        // Default: Dark Mode (THEME_DARK)
        strcpy(current_theme.name, "Mithl Dark");
        
        current_theme.window_bg           = 0xFF2D2D2D; // VS Code Dark
        current_theme.window_title_active = 0xFF3C3C3C;
        current_theme.window_title_inactive=0xFF252526;
        current_theme.window_border       = 0xFF444444; 
        current_theme.window_text         = 0xFFEEEEEE; // White-ish
        
        current_theme.desktop_bg          = 0xFF000000; // Black (User has wallpaper usually)
        current_theme.taskbar_bg          = 0x40FFFFFF; // Glass
        current_theme.start_btn           = 0xFFEEEEEE; // White Icon
        current_theme.bubble_bg           = 0x40FFFFFF; // Glass (Matches Dock)
        
        current_theme.accent_color        = 0xFF007ACC; // Blue Accent
        current_theme.selection_bg        = 0xFF264F78;
        
        current_theme.terminal_bg         = 0xFF1E1E1E; // Standard Dark
        current_theme.terminal_fg         = 0xFFFFFFFF; // White
    }
}
