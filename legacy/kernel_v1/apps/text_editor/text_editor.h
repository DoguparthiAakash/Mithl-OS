#ifndef TEXT_EDITOR_H
#define TEXT_EDITOR_H

#include "gui.h"

// Define editor structure (opaque or minimal)
// Based on text_editor.c usage
#define MAX_EDITOR_LINES 100
#define MAX_LINE_LENGTH 80
#define EDITOR_MAX_FILENAME 64
#define MAX_EDITOR_BUFFER 8192

typedef enum {
    MODE_NORMAL,
    MODE_INSERT,
    MODE_COMMAND
} EditorMode;

typedef struct {
    gui_window_t *window;
    gui_panel_t *panel; // Content panel
    char lines[MAX_EDITOR_LINES][MAX_LINE_LENGTH];
    int line_count;
    int current_line;
    int current_column;
    int cursor_x;
    int cursor_y;
    char filename[EDITOR_MAX_FILENAME];
    int is_modified;
    int is_open;
    
    // Vim-like additions
    EditorMode mode;
    char command_buffer[64];
    int cmd_len;
} text_editor_t;

void text_editor_init(void);
void text_editor_open(const char* filename);
void text_editor_show(void);

#endif
