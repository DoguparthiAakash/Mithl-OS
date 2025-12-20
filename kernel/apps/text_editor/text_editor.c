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
    editor->mode = MODE_NORMAL; // Default to Normal
    editor->cmd_len = 0;
    memset(editor->command_buffer, 0, sizeof(editor->command_buffer));
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
    editor->mode = MODE_NORMAL;
    editor->cmd_len = 0;
    
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
    
    return 0;
}
/* Move cursor */
int editor_move_cursor(text_editor_t *editor, int dx, int dy) {
    if (!editor->is_open) return -1;
    
    // Y Movement
    editor->current_line += dy;
    if (editor->current_line < 0) editor->current_line = 0;
    if (editor->current_line >= editor->line_count) editor->current_line = editor->line_count - 1;
    
    // X Movement
    editor->current_column += dx;
    if (editor->current_column < 0) {
        if (editor->current_line > 0) {
            // Wrap up to end of previous line
            editor->current_line--;
            editor->current_column = strlen(editor->lines[editor->current_line]);
        } else {
            editor->current_column = 0;
        }
    }
    
    int len = strlen(editor->lines[editor->current_line]);
    if (editor->current_column > len) {
        if (dx > 0 && editor->current_line < editor->line_count - 1) {
            // Wrap down to start of next line
            editor->current_line++;
            editor->current_column = 0;
        } else {
            editor->current_column = len; // Clamp
        }
    }
    
    // Update Draw Cursor logic (simplified)
    // Scroll view if needed (TODO)
    
    return 0;
}

/* GUI Implementation */

static void draw_editor_toolbar(gui_window_t *win) {
    int start_y = win->base.bounds.y + 24;
    int w = win->base.bounds.width;
    
    // Background
    draw_rect((rect_t){win->base.bounds.x, start_y, w, 30}, 0xFFF0F0F0);
    draw_rect_outline(win->base.bounds.x, start_y + 29, w, 1, 0xFFCCCCCC);
    
    // Menu Items
    draw_text_sf_mono("File", win->base.bounds.x + 10, start_y + 8, 0xFF000000);
    draw_text_sf_mono("Edit", win->base.bounds.x + 50, start_y + 8, 0xFF000000);
    draw_text_sf_mono("View", win->base.bounds.x + 90, start_y + 8, 0xFF000000);
    
    // Gear Icon
    draw_text_sf_mono("*", win->base.bounds.x + w - 24, start_y + 8, 0xFF555555);
}

static void draw_editor_status_bar(gui_window_t *win, text_editor_t *editor) {
    int h = win->base.bounds.height;
    int y = win->base.bounds.y + h - 20;
    int w = win->base.bounds.width;
    
    draw_rect((rect_t){win->base.bounds.x, y, w, 20}, 0xFF0078D4); // Blue status
    
    // Info
    // Warning: sprintf missing, handle manually or mockup
    draw_text_sf_mono("UTF-8", win->base.bounds.x + w - 50, y + 4, 0xFFFFFFFF);
    draw_text_sf_mono("Ln 1, Col 1", win->base.bounds.x + 10, y + 4, 0xFFFFFFFF);
}

void editor_draw_content(gui_element_t *element) {
    gui_window_t *win = (gui_window_t*)element;
    text_editor_t *editor = &main_editor; // Assuming singleton
    
    // Toolbar
    draw_editor_toolbar(win);
    
    // Content Area
    int start_y = win->base.bounds.y + 24 + 30; // Title + Toolbar
    int start_x = win->base.bounds.x + 2;
    int content_h = win->base.bounds.height - 24 - 30 - 20; // -Title -Toolbar -Status
    
    draw_rect((rect_t){win->base.bounds.x, start_y, win->base.bounds.width, content_h}, 0xFFFFFFFF); // Paper White
    
    // Draw Lines
    int line_h = 16;
    for (int i = 0; i < editor->line_count; i++) {
        int y = start_y + i * line_h;
        if (y > start_y + content_h) break;
        
        draw_text_sf_mono(editor->lines[i], start_x + 30, y, 0xFF000000); // +30 for Gutter
        
        // Draw Line Number in Gutter (Gray)
        // Mockup "1"
        if (i==0) draw_text_sf_mono("1", start_x + 5, y, 0xFFCCCCCC);
    }
    
    // Draw Cursor
    int cx = start_x + 30 + (editor->current_column * 8); // Assume 8px width mono
    int cy = start_y + (editor->current_line * line_h);
    draw_rect((rect_t){cx, cy, 2, line_h}, 0xFF000000); // Black cursor
    
    // Status Bar
    draw_editor_status_bar(win, editor);
}

void editor_handle_event(gui_element_t *element, gui_event_t *event) {
    text_editor_t *editor = &main_editor;
    
    if (event->type == GUI_EVENT_KEY_PRESS) {
        uint8_t key = (uint8_t)event->keyboard.key;
        char c = (char)key; // Basic mapping
        
        // Handle Control Keys
        if (key == 0x08) { // Backspace
            editor_delete_char(editor);
        }
        else if (key == 0x0A) { // Enter
            editor_new_line(editor);
        }
        else if (key == 0x4B) { // Left arrow (scan code dependent, simplified)
             editor_move_cursor(editor, -1, 0);
        }
        else if (key == 0x4D) { // Right arrow
             editor_move_cursor(editor, 1, 0);
        }
        else if (key == 0x48) { // Up arrow
             editor_move_cursor(editor, 0, -1);
        }
        else if (key == 0x50) { // Down arrow
             editor_move_cursor(editor, 0, 1);
        }
        // Save (Ctrl+S assumption or shortcut)
        // ...
        
        // Printable Chars
        else if (c >= 32 && c <= 126) {
             editor_insert_char(editor, c);
        }
        
        gui_mgr.needs_redraw = 1;
    }
}

void text_editor_show(void) {
    if (!main_editor.window) {
        // Init editor first
        text_editor_init();
        
        // Create Window
        gui_window_t *win = gui_create_window("Notepad", 150, 150, 600, 400);
        win->base.draw = editor_draw_content;
        win->base.event_handler = editor_handle_event;
        
        main_editor.window = win;
        editor_new_file(&main_editor);
        
        gui_add_element(gui_mgr.root, (gui_element_t*)win);
    }
    
    gui_bring_to_front((gui_element_t*)main_editor.window);
    main_editor.window->base.flags &= ~GUI_FLAG_HIDDEN;
    gui_mgr.needs_redraw = 1;
}    editor->line_count++;
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
        if (main_editor.window->base.bounds.x < 0) {
             main_editor.window->base.bounds.x = 200;
             main_editor.window->base.bounds.y = 200;
        }
        gui_bring_to_front((gui_element_t*)main_editor.window);
        gui_mgr.focused_element = (gui_element_t*)main_editor.window;
        gui_mgr.needs_redraw = 1;
        return;
    }
    
    text_editor_init();
    
    main_editor.window = gui_create_window("Text Editor", 200, 200, 500, 400);
    // Add to GUI root (Crucial for visibility!)
    if (main_editor.window && gui_mgr.root) {
        gui_add_element(gui_mgr.root, (gui_element_t*)main_editor.window);
    }

    // Panel Logic
    main_editor.panel = gui_create_panel(0, 0, 500, 400 - 48); // Init bounds
    if (!main_editor.panel) return;
    
    main_editor.panel->base.draw = editor_draw_content;
    main_editor.panel->base.event_handler = editor_handle_event;
    main_editor.panel->base.background_color = 0xFFFFFF;
    
    gui_window_add_tab(main_editor.window, "Untitled", (gui_element_t*)main_editor.panel);
    
    main_editor.is_open = 1;
    
    // Ensure visibility
    if (main_editor.window->base.bounds.x < 0) {
        main_editor.window->base.bounds.x = 200;
        main_editor.window->base.bounds.y = 200;
    }
    
    // Bring to front and focus
    gui_bring_to_front((gui_element_t*)main_editor.window);
    gui_mgr.focused_element = (gui_element_t*)main_editor.window;
    gui_mgr.needs_redraw = 1;
}

static void editor_draw_content(gui_renderer_t *renderer, gui_element_t *element) {
    (void)renderer;
    
    // 0. Sync Position with Window
    if (main_editor.window) {
        if (main_editor.window->base.bounds.x < -500) return;
        
        rect_t *w_r = &main_editor.window->base.bounds;
        int title_h = 30;
        element->bounds.x = w_r->x;
        element->bounds.y = w_r->y + title_h;
        element->bounds.width = w_r->width;
        element->bounds.height = w_r->height - title_h;
    }

    rect_t b = element->bounds;
    
    // Windows 11 Notepad Style:
    // [Tab Bar Area (handled by window tabs usually, but we can enhance)]
    // [Menu Bar + Gear]
    // [Text Area]
    // [Status Bar]

    // 1. Background
    draw_rect_filled(b, 0xFFFFFFFF);
    
    // --- Menu Bar / Toolbar ---
    int tool_h = 32;
    rect_t toolbar = {b.x, b.y, b.width, tool_h};
    draw_rect_filled(toolbar, 0xFFF9F9F9);
    draw_line(b.x, b.y + tool_h, b.x + b.width, b.y + tool_h, 0xFFE5E5E5);
    
    // Menu Items
    int mx = b.x + 10;
    int my = b.y + 10;
    draw_text("File", mx, my, 0xFF404040, 0); mx += 40;
    draw_text("Edit", mx, my, 0xFF404040, 0); mx += 40;
    draw_text("View", mx, my, 0xFF404040, 0); mx += 40;
    
    // Gear Icon (Right aligned)
    int gear_x = b.x + b.width - 30;
    draw_circle_filled(gear_x, my + 4, 3, 0xFF606060); // Hub
    // Teeth (simplified)
    draw_circle(gear_x, my + 4, 7, 0xFF606060);

    // --- Status Bar (Bottom) ---
    int status_h = 24;
    int status_y = b.y + b.height - status_h;
    
    draw_rect_filled((rect_t){b.x, status_y, b.width, status_h}, 0xFFF9F9F9);
    draw_line(b.x, status_y, b.x + b.width, status_y, 0xFFE5E5E5);
    
    // Status Text
    draw_text("Ln", b.x + 10, status_y + 6, 0xFF606060, 0);
    draw_text("1, Col 1", b.x + 30, status_y + 6, 0xFF606060, 0); // Placeholder
    
    draw_text("UTF-8", b.x + b.width - 60, status_y + 6, 0xFF606060, 0);

    // --- Text Area ---
    int text_y_start = b.y + tool_h;
    int text_h = b.height - tool_h - status_h;
    
    // Render lines
    int line_h = 16;
    int start_line = 0; // Scroll support later
    int visible_lines = text_h / line_h;
    
    for (int i = 0; i < visible_lines && (start_line + i) < main_editor.line_count; i++) {
        int ly = text_y_start + i * line_h + 2;
        char *line = main_editor.lines[start_line + i];
        
        // Draw mono text (CORRECTED: 4 args)
        if (line[0] != '\0') {
             draw_text_sf_mono(line, b.x + 5, ly, 0xFF000000);
        }
        
        // Draw Cursor
        if (start_line + i == main_editor.current_line) {
             if (main_editor.is_open) { 
                 int cursor_px = b.x + 5 + (main_editor.cursor_x * 9); 
                 draw_rect_filled((rect_t){cursor_px, ly, 2, 14}, 0x000000);
             }
        }
    }
}

static void editor_handle_event(gui_element_t *element, gui_event_t *event) {
    (void)element;
    // gui_window_event_handler(element, event); // REMOVED
    
    if (main_editor.window && main_editor.window->base.bounds.x == -9999) {
        main_editor.is_open = 0; // Sync state
        return;
    }
    
    if (event->type == GUI_EVENT_KEY_PRESS) {
         char key = event->keyboard.key;
         
         // Mode Switching Logic
         if (key == 27) { // ESC
             if (main_editor.mode == MODE_INSERT || main_editor.mode == MODE_COMMAND) {
                 main_editor.mode = MODE_NORMAL;
                 main_editor.cmd_len = 0;
                 gui_mgr.needs_redraw = 1;
                 return;
             }
         }
         
         if (main_editor.mode == MODE_NORMAL) {
             // Navigation (hjkl)
             if (key == 'h') { if (main_editor.current_column > 0) editor_move_cursor(&main_editor, main_editor.current_column - 1, main_editor.current_line); }
             else if (key == 'j') { if (main_editor.current_line < main_editor.line_count - 1) editor_move_cursor(&main_editor, main_editor.current_column, main_editor.current_line + 1); }
             else if (key == 'k') { if (main_editor.current_line > 0) editor_move_cursor(&main_editor, main_editor.current_column, main_editor.current_line - 1); }
             else if (key == 'l') { if (main_editor.current_column < (int)strlen(main_editor.lines[main_editor.current_line])) editor_move_cursor(&main_editor, main_editor.current_column + 1, main_editor.current_line); }
             
             // Enter Insert Mode
             else if (key == 'i') {
                 main_editor.mode = MODE_INSERT;
             }
             // Enter Command Mode
             else if (key == ':') {
                 main_editor.mode = MODE_COMMAND;
                 main_editor.cmd_len = 0;
                 main_editor.command_buffer[0] = ':';
                 main_editor.cmd_len = 1;
                 main_editor.command_buffer[1] = 0;
             }
             // Delete char
             else if (key == 'x') {
                 editor_delete_char(&main_editor);
             }
             
             gui_mgr.needs_redraw = 1;
         }
         else if (main_editor.mode == MODE_INSERT) {
             // Typing
             if (key >= 32 && key <= 126) {
                  editor_insert_char(&main_editor, key);
             }
             else if (key == '\n' || key == 13) {
                  editor_new_line(&main_editor);
             }
             else if (key == '\b' || key == 8) {
                  editor_delete_char(&main_editor);
             }
             gui_mgr.needs_redraw = 1;
         }
         else if (main_editor.mode == MODE_COMMAND) {
             // Command entry
             if (key == '\n' || key == 13) {
                 // Execute command
                 if (strcmp(main_editor.command_buffer, ":w") == 0) {
                     if (editor_save_file(&main_editor) >= 0) {
                         // Indicate success?
                     }
                 }
                 else if (strcmp(main_editor.command_buffer, ":q") == 0) {
                     main_editor.is_open = 0;
                 }
                 else if (strcmp(main_editor.command_buffer, ":wq") == 0) {
                     editor_save_file(&main_editor);
                     main_editor.is_open = 0;
                 }
                 
                 main_editor.mode = MODE_NORMAL;
                 main_editor.cmd_len = 0;
                 gui_mgr.needs_redraw = 1;
             }
             else if (key == '\b' || key == 8) {
                 if (main_editor.cmd_len > 0) {
                     main_editor.cmd_len--;
                     main_editor.command_buffer[main_editor.cmd_len] = 0;
                     if (main_editor.cmd_len == 0) main_editor.mode = MODE_NORMAL; // Cancel if empty
                     gui_mgr.needs_redraw = 1;
                 }
             }
             else if (key >= 32 && key <= 126) {
                 if (main_editor.cmd_len < 63) {
                     main_editor.command_buffer[main_editor.cmd_len++] = key;
                     main_editor.command_buffer[main_editor.cmd_len] = 0;
                     gui_mgr.needs_redraw = 1;
                 }
             }
         }
    }
}


/* Draw functions - placeholders for now */
void editor_draw_interface(text_editor_t *editor) { (void)editor; }
void editor_draw_text_area(text_editor_t *editor) { (void)editor; }
void editor_draw_status_bar(text_editor_t *editor) { (void)editor; }
void editor_draw_menu_bar(text_editor_t *editor) { (void)editor; }
