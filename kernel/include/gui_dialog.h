#ifndef GUI_DIALOG_H
#define GUI_DIALOG_H

#include "gui.h"

// Dialog callback types
typedef void (*dialog_callback_t)(const char* result);  // For input dialogs
typedef void (*confirm_callback_t)(int confirmed);      // For confirm dialogs

// Show input dialog (modal)
void gui_show_input_dialog(const char* title, const char* prompt, dialog_callback_t callback);

// Show confirmation dialog (modal)
void gui_show_confirm_dialog(const char* title, const char* message, confirm_callback_t callback);

// Show alert dialog (modal)
void gui_show_alert_dialog(const char* title, const char* message);

// Internal: Check if dialog is active
int gui_dialog_is_active(void);

// Internal: Handle dialog events
void gui_dialog_handle_event(gui_event_t* event);

// Internal: Draw active dialog
void gui_dialog_draw(void);

#endif
