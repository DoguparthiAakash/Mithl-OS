#include "gui_dialog.h"
#include "graphics.h"
#include "string.h"
#include "memory.h"
#include "console.h"

// Dialog state
static struct {
    int active;
    int type;  // 0=none, 1=input, 2=confirm, 3=alert
    char title[64];
    char message[256];
    char input_buffer[128];
    int input_cursor;
    dialog_callback_t input_callback;
    confirm_callback_t confirm_callback;
} dialog_state = {0};

// Show input dialog
void gui_show_input_dialog(const char* title, const char* prompt, dialog_callback_t callback) {
    dialog_state.active = 1;
    dialog_state.type = 1;
    strncpy(dialog_state.title, title, 63);
    dialog_state.title[63] = 0;
    strncpy(dialog_state.message, prompt, 255);
    dialog_state.message[255] = 0;
    dialog_state.input_buffer[0] = 0;
    dialog_state.input_cursor = 0;
    dialog_state.input_callback = callback;
}

// Show confirmation dialog
void gui_show_confirm_dialog(const char* title, const char* message, confirm_callback_t callback) {
    dialog_state.active = 1;
    dialog_state.type = 2;
    strncpy(dialog_state.title, title, 63);
    dialog_state.title[63] = 0;
    strncpy(dialog_state.message, message, 255);
    dialog_state.message[255] = 0;
    dialog_state.confirm_callback = callback;
}

// Show alert dialog
void gui_show_alert_dialog(const char* title, const char* message) {
    dialog_state.active = 1;
    dialog_state.type = 3;
    strncpy(dialog_state.title, title, 63);
    dialog_state.title[63] = 0;
    strncpy(dialog_state.message, message, 255);
    dialog_state.message[255] = 0;
}

// Check if dialog is active
int gui_dialog_is_active(void) {
    return dialog_state.active;
}

// Handle dialog events
void gui_dialog_handle_event(gui_event_t* event) {
    if (!dialog_state.active) return;
    
    if (event->type == GUI_EVENT_KEY_PRESS) {
        if (dialog_state.type == 1) {  // Input dialog
            char key = event->keyboard.key;
            
            if (key == '\n') {  // Enter
                // Submit
                if (dialog_state.input_callback) {
                    dialog_state.input_callback(dialog_state.input_buffer);
                }
                dialog_state.active = 0;
            } else if (key == 27) {  // Escape
                dialog_state.active = 0;
            } else if (key == '\b') {  // Backspace
                if (dialog_state.input_cursor > 0) {
                    dialog_state.input_cursor--;
                    dialog_state.input_buffer[dialog_state.input_cursor] = 0;
                }
            } else if (key >= 32 && key < 127) {  // Printable characters
                if (dialog_state.input_cursor < 127) {
                    dialog_state.input_buffer[dialog_state.input_cursor++] = key;
                    dialog_state.input_buffer[dialog_state.input_cursor] = 0;
                }
            }
        } else if (dialog_state.type == 2) {  // Confirm dialog
            char key = event->keyboard.key;
            if (key == 'y' || key == 'Y' || key == '\n') {
                if (dialog_state.confirm_callback) {
                    dialog_state.confirm_callback(1);
                }
                dialog_state.active = 0;
            } else if (key == 'n' || key == 'N' || key == 27) {
                if (dialog_state.confirm_callback) {
                    dialog_state.confirm_callback(0);
                }
                dialog_state.active = 0;
            }
        } else if (dialog_state.type == 3) {  // Alert dialog
            dialog_state.active = 0;
        }
    }
}

// Draw active dialog
void gui_dialog_draw(void) {
    if (!dialog_state.active) return;
    
    // Get screen dimensions from GUI manager
    extern struct gui_manager gui_mgr;
    int screen_w = gui_mgr.screen_width;
    int screen_h = gui_mgr.screen_height;
    
    // Dialog dimensions
    int dialog_w = 400;
    int dialog_h = 200;
    int dialog_x = (screen_w - dialog_w) / 2;
    int dialog_y = (screen_h - dialog_h) / 2;
    
    // Draw overlay (semi-transparent background)
    draw_rect_filled((rect_t){0, 0, screen_w, screen_h}, 0x80000000);
    
    // Draw dialog box
    draw_rect_filled((rect_t){dialog_x, dialog_y, dialog_w, dialog_h}, 0xFFF5F5F5);
    draw_rect_outline(dialog_x, dialog_y, dialog_w, dialog_h, 0xFF888888);
    
    // Draw title bar
    draw_rect_filled((rect_t){dialog_x, dialog_y, dialog_w, 30}, 0xFFE0E0E0);
    draw_text_sf_mono(dialog_state.title, dialog_x + 10, dialog_y + 8, 0xFF000000);
    
    // Draw message
    draw_text_sf_mono(dialog_state.message, dialog_x + 20, dialog_y + 50, 0xFF333333);
    
    if (dialog_state.type == 1) {  // Input dialog
        // Draw input box
        int input_y = dialog_y + 90;
        draw_rect_filled((rect_t){dialog_x + 20, input_y, dialog_w - 40, 30}, 0xFFFFFFFF);
        draw_rect_outline(dialog_x + 20, input_y, dialog_w - 40, 30, 0xFF999999);
        
        // Draw input text
        draw_text_sf_mono(dialog_state.input_buffer, dialog_x + 25, input_y + 8, 0xFF000000);
        
        // Draw cursor (blinking would require timer)
        int cursor_x = dialog_x + 25 + (dialog_state.input_cursor * 9);
        draw_line(cursor_x, input_y + 5, cursor_x, input_y + 25, 0xFF000000);
        
        // Draw hint
        draw_text_sf_mono("Press Enter to confirm, Esc to cancel", dialog_x + 20, dialog_y + 140, 0xFF666666);
        
    } else if (dialog_state.type == 2) {  // Confirm dialog
        // Draw buttons
        int btn_y = dialog_y + 130;
        int btn_w = 80;
        int btn_h = 30;
        
        // Yes button
        draw_rect_filled((rect_t){dialog_x + 80, btn_y, btn_w, btn_h}, 0xFF0078D4);
        draw_text_sf_mono("Yes (Y)", dialog_x + 90, btn_y + 8, 0xFFFFFFFF);
        
        // No button
        draw_rect_filled((rect_t){dialog_x + 240, btn_y, btn_w, btn_h}, 0xFFCCCCCC);
        draw_text_sf_mono("No (N)", dialog_x + 252, btn_y + 8, 0xFF000000);
        
    } else if (dialog_state.type == 3) {  // Alert dialog
        // Draw OK button
        int btn_y = dialog_y + 130;
        int btn_w = 80;
        int btn_h = 30;
        
        draw_rect_filled((rect_t){dialog_x + 160, btn_y, btn_w, btn_h}, 0xFF0078D4);
        draw_text_sf_mono("OK", dialog_x + 185, btn_y + 8, 0xFFFFFFFF);
        
        draw_text_sf_mono("Press any key to close", dialog_x + 120, dialog_y + 170, 0xFF666666);
    }
}
