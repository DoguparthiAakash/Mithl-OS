#ifndef TERMINAL_H
#define TERMINAL_H

#include "gui.h"

// Terminal dimensions
#define TERM_ROWS 25
#define TERM_COLS 80

typedef struct
{
    gui_window_t *window;
    gui_panel_t *panel; // Content panel
    char buffer[TERM_ROWS][TERM_COLS];
    int cursor_x;
    int cursor_y;
    uint32_t bg_color;
    uint32_t fg_color;
    char input_buffer[256];
    int input_len;
} terminal_t;

void terminal_init(void);
void terminal_show(void);
terminal_t *terminal_create(void);
void terminal_run_command(terminal_t *term, const char *command);
void terminal_run_command_active(const char *command);

#endif // TERMINAL_H
