#include "settings.h"
#include "../../include/gui.h"

// Forward declaration
extern gui_manager_t gui_mgr;

static gui_window_t *settings_window = 0;

void settings_show(void) {
    if (settings_window) {
        gui_bring_to_front((gui_element_t*)settings_window);
        return;
    }
    
    settings_window = gui_create_window("Settings", 150, 150, 600, 400);
    if (settings_window && gui_mgr.root) {
        gui_add_element(gui_mgr.root, (gui_element_t*)settings_window);
    }
    
    // Create main panel
    gui_panel_t *panel = gui_create_panel(0, 0, 600, 400-24);
    if(panel) {
        panel->base.background_color = 0xFFF0F0F0;
        
        // Add tab
        gui_window_add_tab(settings_window, "System", (gui_element_t*)panel);
        
        // Add some dummy text to the panel?
        // Need a layout or label widget. 
        // For now, just a plain panel is enough to pass build.
    }
}
