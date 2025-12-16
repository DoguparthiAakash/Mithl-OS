#include "gui.h"
#include "text_editor.h"
#include "string.h"
#include "filesystem.h"

/* Define size_t for freestanding environment */
typedef unsigned int size_t;

/* Global editor instance */
text_editor_t main_editor = { 0 };

// Forward declarations
int editor_init(text_editor_t *editor);
int editor_open_file(text_editor_t *editor, const char *filename);
int editor_close_file(text_editor_t *editor);
int editor_save_file(text_editor_t *editor);
int editor_set_content(text_editor_t *editor, const char *content);
int editor_insert_char(text_editor_t *editor, char c);
int editor_new_line(text_editor_t *editor);
int editor_delete_char(text_editor_t *editor);
int editor_move_cursor(text_editor_t *editor, int x, int y);  

// Local helpers removed, using string.h

// Basic initialization wrapper
void text_editor_init(void) {
    editor_init(&main_editor);
}

/* Initialize text editor structure */
int editor_init(text_editor_t *editor)
{
    memset(editor, 0, sizeof(text_editor_t));
    editor->current_line = 0;
    editor->current_column = 0;
    editor->cursor_x = 0;
    editor->cursor_y = 0;
    editor->is_modified = 0;
    editor->is_open = 0;
    return 0;
}

/* Open a file in the editor */
int editor_open_file(text_editor_t *editor, const char *filename)
{
    if (editor->is_open) {
        editor_close_file(editor);
    }
    
    memcpy(editor->filename, filename, strlen(filename) + 1);
    
    // Read file content
    char buffer[MAX_EDITOR_BUFFER];
    int bytes_read = fs_read_file(filename, buffer, MAX_EDITOR_BUFFER);
    
    if (bytes_read > 0) {
        editor_set_content(editor, buffer);
    } else {
        // File doesn't exist or is empty, start with empty content
        editor->line_count = 1;
        memset(editor->lines[0], 0, MAX_LINE_LENGTH);
    }
    
    editor->is_open = 1;
    editor->is_modified = 0;
    return 0;
}

/* Save the current file */
int editor_save_file(text_editor_t *editor)
{
    if (!editor->is_open) return -1;
    
    // Convert lines to buffer
    char buffer[MAX_EDITOR_BUFFER];
    int offset = 0;
    
    for (int i = 0; i < editor->line_count; i++) {
        int line_len = strlen(editor->lines[i]);
        memcpy(buffer + offset, editor->lines[i], line_len);
        offset += line_len;
        
        if (i < editor->line_count - 1) {
            buffer[offset++] = '\n';
        }
    }
    
    // Write to filesystem
    int result = fs_write_file(editor->filename, buffer, offset);
    if (result > 0) {
        editor->is_modified = 0;
    }
    
    return result;
}

/* Create a new file */
int editor_new_file(text_editor_t *editor)
{
    if (editor->is_open) {
        editor_close_file(editor);
    }
    
    memset(editor->filename, 0, EDITOR_MAX_FILENAME);
    editor->line_count = 1;
    memset(editor->lines[0], 0, MAX_LINE_LENGTH);
    editor->current_line = 0;
    editor->current_column = 0;
    editor->cursor_x = 0;
    editor->cursor_y = 0;
    editor->is_modified = 0;
    editor->is_open = 1;
    
    return 0;
}

/* Close the current file */
int editor_close_file(text_editor_t *editor)
{
    if (!editor->is_open) return 0;
    
    if (editor->is_modified) {
        // Auto-save for now
        editor_save_file(editor);
    }
    
    editor->is_open = 0;
    editor->is_modified = 0;
    return 0;
}

/* Insert a character at cursor position */
int editor_insert_char(text_editor_t *editor, char c)
{
    if (!editor->is_open) return -1;
    
    if (c == '\n') {
        return editor_new_line(editor);
    }
    
    if (editor->current_column >= MAX_LINE_LENGTH - 1) return -1;
    
    // Shift characters to make room
    for (int i = MAX_LINE_LENGTH - 2; i > editor->current_column; i--) {
        editor->lines[editor->current_line][i] = editor->lines[editor->current_line][i - 1];
    }
    
    editor->lines[editor->current_line][editor->current_column] = c;
    editor->current_column++;
    editor->cursor_x++;
    editor->is_modified = 1;
    
    return 0;
}

/* Delete character at cursor position */
int editor_delete_char(text_editor_t *editor)
{
    if (!editor->is_open) return -1;
    if (editor->current_column <= 0) return -1;
    
    // Shift characters to remove the one at cursor
    for (int i = editor->current_column - 1; i < MAX_LINE_LENGTH - 1; i++) {
        editor->lines[editor->current_line][i] = editor->lines[editor->current_line][i + 1];
    }
    
    editor->current_column--;
    editor->cursor_x--;
    editor->is_modified = 1;
    
    return 0;
}

/* Create a new line */
int editor_new_line(text_editor_t *editor)
{
    if (!editor->is_open) return -1;
    if (editor->line_count >= MAX_EDITOR_LINES - 1) return -1;
    
    // Shift lines down
    for (int i = editor->line_count; i > editor->current_line + 1; i--) {
        memcpy(editor->lines[i], editor->lines[i - 1], MAX_LINE_LENGTH);
    }
    
    // Split current line
    int remaining_chars = strlen(editor->lines[editor->current_line]) - editor->current_column;
    if (remaining_chars > 0) {
        memcpy(editor->lines[editor->current_line + 1], 
               editor->lines[editor->current_line] + editor->current_column, 
               remaining_chars);
        editor->lines[editor->current_line][editor->current_column] = '\0';
    } else {
        memset(editor->lines[editor->current_line + 1], 0, MAX_LINE_LENGTH);
    }
    
    editor->line_count++;
    editor->current_line++;
    editor->current_column = 0;
    editor->cursor_x = 0;
    editor->cursor_y++;
    editor->is_modified = 1;
    
    return 0;
}

/* Move cursor to specified position */
int editor_move_cursor(text_editor_t *editor, int x, int y)
{
    if (!editor->is_open) return -1;
    
    if (y >= 0 && y < editor->line_count) {
        editor->current_line = y;
        editor->cursor_y = y;
    }
    
    if (x >= 0 && x < MAX_LINE_LENGTH) {
        editor->current_column = x;
        editor->cursor_x = x;
    }
    
    return 0;
}

/* Get editor content as string */
int editor_get_content(text_editor_t *editor, char *buffer, int buffer_size)
{
    if (!editor->is_open) return -1;
    
    int offset = 0;
    for (int i = 0; i < editor->line_count && offset < buffer_size - 1; i++) {
        int line_len = strlen(editor->lines[i]);
        if (offset + line_len >= buffer_size - 1) break;
        
        memcpy(buffer + offset, editor->lines[i], line_len);
        offset += line_len;
        
        if (i < editor->line_count - 1) {
            buffer[offset++] = '\n';
        }
    }
    
    buffer[offset] = '\0';
    return offset;
}

/* Set editor content from string */
int editor_set_content(text_editor_t *editor, const char *content)
{
    if (!editor->is_open) return -1;
    
    memset(editor->lines, 0, sizeof(editor->lines));
    editor->line_count = 0;
    
    int line_start = 0;
    int content_len = strlen(content);
    
    for (int i = 0; i < content_len && editor->line_count < MAX_EDITOR_LINES; i++) {
        if (content[i] == '\n' || i == content_len - 1) {
            int line_len = i - line_start;
            if (i == content_len - 1 && content[i] != '\n') line_len++;
            
            if (line_len > 0) {
                int copy_len = (line_len < MAX_LINE_LENGTH - 1) ? line_len : MAX_LINE_LENGTH - 1;
                memcpy(editor->lines[editor->line_count], content + line_start, copy_len);
                editor->lines[editor->line_count][copy_len] = '\0';
            }
            
            editor->line_count++;
            line_start = i + 1;
        }
    }
    
    if (editor->line_count == 0) {
        editor->line_count = 1;
        memset(editor->lines[0], 0, MAX_LINE_LENGTH);
    }
    
    return 0;
}

// Wrapper to open file from File Manager
void text_editor_open(const char* filename) {
    if (!main_editor.is_open) {
        text_editor_init();
    }
    
    // Create window if needed
    text_editor_show();
    
    // Load content
    editor_open_file(&main_editor, filename);
}

static void editor_draw_content(gui_renderer_t *renderer, gui_element_t *element);
static void editor_handle_event(gui_element_t *element, gui_event_t *event);

void text_editor_show(void) {
    if (main_editor.window) {
        // Just focus
        gui_mgr.focused_element = (gui_element_t*)main_editor.window;
        return;
    }
    
    text_editor_init();
    
    main_editor.window = gui_create_window("Text Editor", 200, 200, 500, 400);
    // Panel Logic
    main_editor.panel = gui_create_panel(0, 0, 500, 400 - 48); // Init bounds
    if (!main_editor.panel) return;
    
    main_editor.panel->base.draw = editor_draw_content;
    main_editor.panel->base.event_handler = editor_handle_event;
    main_editor.panel->base.background_color = 0xFFFFFF;
    
    gui_window_add_tab(main_editor.window, "Untitled", (gui_element_t*)main_editor.panel);
    
    main_editor.is_open = 1;
}

static void editor_draw_content(gui_renderer_t *renderer, gui_element_t *element) {
    // 1. Standard Frame
    // 1. Background
    draw_rect_filled(element->bounds, 0xFFFFFF);
    
    rect_t bounds = element->bounds;
    
    // Update window title dynamically if needed (gui_draw_window uses element->text)
    // We update it here or in event loop.
    char title[100];
    if (main_editor.filename[0]) {
         strcpy(title, "Text Editor - ");
         strcat(title, main_editor.filename);
    } else {
         strcpy(title, "Text Editor - Untitled");
    }
    // Note: element->text points to a buffer? or allocated?
    // In gui.c gui_create_window allocated it.
    // If we change it, we should re-allocate or assume bounds. 
    // For safety, let's just draw subtitle for now as gui_draw_window draws element->text (static).
    // Or assume element->text is large enough?
    // Let's just draw the standard title (Text Editor) in gui_draw_window, 
    // and draw the filename in the sub-header.
    
    // Adjust for panel
    
    // Check main_editor.is_open
    if (!main_editor.is_open) return;
    
    int start_y = bounds.y + 5;
    
    // Draw Sub-header (Filename)
    draw_rect_filled((rect_t){bounds.x + 2, start_y, bounds.width - 4, 16}, 0xEEEEEE);
    draw_text(title, bounds.x + 5, start_y + 4, 0x000000, 10);
    
    // 3. Text Area
    start_y += 20;
    int line_h = 14;
    
    for (int i = 0; i < main_editor.line_count; i++) {
        int y = start_y + i * line_h;
        if (y + line_h > bounds.y + bounds.height) break;
        
        draw_text(main_editor.lines[i], bounds.x + 5, y, 0x000000, 10);
        
        // Cursor
        if (i == main_editor.current_line) {
             // Calculate cursor X
             int cx = bounds.x + 5 + main_editor.current_column * 8; // approx font width
             draw_rect((rect_t){cx, y, 2, 12}, 0x000000);
        }
    }
}

static void editor_handle_event(gui_element_t *element, gui_event_t *event) {
    // gui_window_event_handler(element, event); // REMOVED
    
    if (main_editor.window && main_editor.window->base.bounds.x == -9999) {
        main_editor.is_open = 0; // Sync state
        return;
    }
    
    if (event->type == GUI_EVENT_KEY_PRESS) {
         // Handle typing
         char key = event->keyboard.key;
         
         if (key >= 32 && key <= 126) {
              editor_insert_char(&main_editor, key);
              gui_mgr.needs_redraw = 1;
         }
         else if (key == '\n' || key == 13) { // Enter
              editor_new_line(&main_editor);
              gui_mgr.needs_redraw = 1;
         }
         else if (key == '\b' || key == 8) { // Backspace
              editor_delete_char(&main_editor);
              gui_mgr.needs_redraw = 1;
         }
    }
}


/* Draw functions - placeholders for now */
void editor_draw_interface(text_editor_t *editor) { (void)editor; }
void editor_draw_text_area(text_editor_t *editor) { (void)editor; }
void editor_draw_status_bar(text_editor_t *editor) { (void)editor; }
void editor_draw_menu_bar(text_editor_t *editor) { (void)editor; }
