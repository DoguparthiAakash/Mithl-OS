#include "gui.h"
#include "text_editor.h"
#include "string.h"
#include "vfs.h" // Use VFS directly now
#include "memory.h"

/* Define size_t for freestanding environment */
typedef unsigned int size_t;

extern fs_node_t *fs_root; // VFS Root

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

void text_editor_init(void) {
    editor_init(&main_editor);
}

int editor_init(text_editor_t *editor)
{
    memset(editor, 0, sizeof(text_editor_t));
    editor->current_line = 0;
    editor->current_column = 0;
    editor->cursor_x = 0;
    editor->cursor_y = 0;
    editor->is_modified = 0;
    editor->is_open = 0;
    editor->mode = MODE_NORMAL; 
    editor->cmd_len = 0;
    memset(editor->command_buffer, 0, sizeof(editor->command_buffer));
    
    // Default dummy lines
    editor->line_count = 1;
    memset(editor->lines[0], 0, MAX_LINE_LENGTH);
    return 0;
}

/* Open a file in the editor using VFS */
int editor_open_file(text_editor_t *editor, const char *filename)
{
    if (editor->is_open) {
        editor_close_file(editor);
    }
    
    if (strlen(filename) >= EDITOR_MAX_FILENAME) return -1;
    strcpy(editor->filename, filename);
    
    // Read file content via VFS
    fs_node_t *node = vfs_resolve_path(filename);
    char *buffer = (char*)memory_alloc(MAX_EDITOR_BUFFER); // Alloc on heap to be safe
    int bytes_read = 0;
    
    if (node) {
        bytes_read = read_fs(node, 0, MAX_EDITOR_BUFFER, (uint8_t*)buffer);
    }
    
    if (bytes_read > 0) {
        buffer[bytes_read] = 0;
        editor_set_content(editor, buffer);
    } else {
        editor->line_count = 1;
        memset(editor->lines[0], 0, MAX_LINE_LENGTH);
    }
    memory_free(buffer);
    
    editor->is_open = 1;
    editor->is_modified = 0;
    return 0;
}

/* Save the current file using VFS */
int editor_save_file(text_editor_t *editor)
{
    if (!editor->is_open) return -1;
    if (strlen(editor->filename) == 0) return -1; // Cannot save without name
    
    // Convert lines to buffer
    char *buffer = (char*)memory_alloc(MAX_EDITOR_BUFFER);
    int offset = 0;
    
    for (int i = 0; i < editor->line_count; i++) {
        int line_len = strlen(editor->lines[i]);
        if (offset + line_len + 1 >= MAX_EDITOR_BUFFER) break;
        memcpy(buffer + offset, editor->lines[i], line_len);
        offset += line_len;
        
        if (i < editor->line_count - 1) {
            buffer[offset++] = '\n';
        }
    }
    
    // Write via VFS
    fs_node_t *node = vfs_resolve_path(editor->filename);
    if (!node) {
        // Create if missing
        // Assume root-relative if slash missing, or absolute path
        char path_buf[128];
        strcpy(path_buf, editor->filename);
        
        // Split parent/child logic (Simplified: assume root if just filename)
        char *last_slash = 0;
        char *p = path_buf; while(*p){ if(*p=='/') last_slash=p; p++; }
        
        if (last_slash) {
            *last_slash = 0;
            char *parent_path = path_buf;
            if (parent_path[0]==0) parent_path="/";
            char *child_name = last_slash+1;
            
            fs_node_t *parent = vfs_resolve_path(parent_path);
            if (parent) {
                create_fs(parent, child_name, 0);
            }
        } else {
            // Root
            extern fs_node_t *fs_root;
            if (fs_root) create_fs(fs_root, path_buf, 0);
        }
        
        // Resolve again
        node = vfs_resolve_path(editor->filename);
    }
    
    int result = -1;
    if (node) {
        result = write_fs(node, 0, offset, (uint8_t*)buffer);
        if (result >= 0) editor->is_modified = 0;
    }
    
    memory_free(buffer);
    return result;
}

int editor_close_file(text_editor_t *editor)
{
    if (!editor->is_open) return 0;
    editor->is_open = 0;
    editor->is_modified = 0;
    return 0;
}

// ... Content Manipulation (Same as before) ...
int editor_set_content(text_editor_t *editor, const char *content) {
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
    if (editor->line_count == 0) { editor->line_count = 1; memset(editor->lines[0], 0, MAX_LINE_LENGTH); }
    return 0;
}

int editor_insert_char(text_editor_t *editor, char c) {
    if (!editor->is_open) return -1;
    if (c == '\n') return editor_new_line(editor);
    if (editor->current_column >= MAX_LINE_LENGTH - 1) return -1;
    for (int i = MAX_LINE_LENGTH - 2; i > editor->current_column; i--) editor->lines[editor->current_line][i] = editor->lines[editor->current_line][i - 1];
    editor->lines[editor->current_line][editor->current_column] = c;
    editor->current_column++; editor->cursor_x++; editor->is_modified = 1;
    return 0;
}

int editor_delete_char(text_editor_t *editor) {
    if (!editor->is_open) return -1;
    if (editor->current_column <= 0) return -1;
    for (int i = editor->current_column - 1; i < MAX_LINE_LENGTH - 1; i++) editor->lines[editor->current_line][i] = editor->lines[editor->current_line][i + 1];
    editor->current_column--; editor->cursor_x--; editor->is_modified = 1;
    return 0;
}

int editor_new_line(text_editor_t *editor) {
    if (!editor->is_open) return -1;
    if (editor->line_count >= MAX_EDITOR_LINES - 1) return -1;
    for (int i = editor->line_count; i > editor->current_line + 1; i--) memcpy(editor->lines[i], editor->lines[i - 1], MAX_LINE_LENGTH);
    int remaining_chars = strlen(editor->lines[editor->current_line]) - editor->current_column;
    if (remaining_chars > 0) {
        memcpy(editor->lines[editor->current_line + 1], editor->lines[editor->current_line] + editor->current_column, remaining_chars);
        editor->lines[editor->current_line][editor->current_column] = '\0';
    } else { memset(editor->lines[editor->current_line + 1], 0, MAX_LINE_LENGTH); }
    editor->line_count++; editor->current_line++; editor->current_column = 0; editor->cursor_x = 0;
    return 0;
}

int editor_move_cursor(text_editor_t *editor, int dx, int dy) {
    if (!editor->is_open) return -1;
    editor->current_line += dy;
    if (editor->current_line < 0) editor->current_line = 0;
    if (editor->current_line >= editor->line_count) editor->current_line = editor->line_count - 1;
    editor->current_column += dx;
    if (editor->current_column < 0) { if (editor->current_line > 0) { editor->current_line--; editor->current_column = strlen(editor->lines[editor->current_line]); } else { editor->current_column = 0; } }
    int len = strlen(editor->lines[editor->current_line]);
    if (editor->current_column > len) { if (dx > 0 && editor->current_line < editor->line_count - 1) { editor->current_line++; editor->current_column = 0; } else { editor->current_column = len; } }
    editor->cursor_x = editor->current_column; editor->cursor_y = editor->current_line;
    return 0;
}

void text_editor_open(const char* filename) {
    if (!main_editor.is_open) text_editor_init();
    text_editor_show();
    editor_open_file(&main_editor, filename);
}

static void editor_draw_content(gui_renderer_t *renderer, gui_element_t *element);
static void editor_handle_event(gui_element_t *element, gui_event_t *event);

void text_editor_show(void) {
    if (main_editor.window) {
        gui_bring_to_front((gui_element_t*)main_editor.window);
        gui_mgr.focused_element = (gui_element_t*)main_editor.window;
        gui_mgr.needs_redraw = 1;
        return;
    }
    text_editor_init();
    main_editor.window = gui_create_window("Text Editor (Notepad)", 300, 300, 500, 400); // Offset to avoid overlap
    main_editor.panel = gui_create_panel(0, 0, 500, 400 - 48); 
    main_editor.panel->base.draw = (void*)editor_draw_content; 
    main_editor.panel->base.event_handler = editor_handle_event;
    main_editor.panel->base.background_color = 0xFFFFFF;
    gui_window_add_tab(main_editor.window, "Editor", (gui_element_t*)main_editor.panel);
    main_editor.is_open = 1;
    gui_bring_to_front((gui_element_t*)main_editor.window);
    gui_mgr.focused_element = (gui_element_t*)main_editor.window;
    gui_mgr.needs_redraw = 1;
    
    // Default welcome
    editor_set_content(&main_editor, "Welcome to Notepad.\nPress 'i' to insert, ESC for mode.\nType :w <name> to save.\nRun 'cc <name>' in terminal to execute.");
}

static void editor_draw_content(gui_renderer_t *renderer, gui_element_t *element) {
    (void)renderer;
    if (main_editor.window) {
        rect_t *w_r = &main_editor.window->base.bounds;
        int title_h = 30;
        element->bounds.x = w_r->x; element->bounds.y = w_r->y + title_h;
        element->bounds.width = w_r->width; element->bounds.height = w_r->height - title_h;
    }
    rect_t b = element->bounds;
    draw_rect_filled(b, 0xFFFFFFFF);
    
    int tool_h = 32;
    rect_t toolbar = {b.x, b.y, b.width, tool_h};
    draw_rect_filled(toolbar, 0xFFE0E0E0);
    draw_line(b.x, b.y + tool_h, b.x + b.width, b.y + tool_h, 0xFF808080);
    
    // Display Filename
    char file_label[128];
    if (strlen(main_editor.filename) > 0) {
        strcpy(file_label, "File: ");
        strcat(file_label, main_editor.filename);
        if (main_editor.is_modified) strcat(file_label, " *");
    } else {
        strcpy(file_label, "File: [Untitled] (Use :w <name>)");
    }
    draw_text(file_label, b.x + 10, b.y + 10, 0xFF000000, 0);

    // Command Bar if Active
    if (main_editor.mode == MODE_COMMAND) {
         draw_rect_filled((rect_t){b.x, b.y + b.height - 20, b.width, 20}, 0xFF303030);
         draw_text_sf_mono(main_editor.command_buffer, b.x+5, b.y + b.height - 15, 0xFFFFFFFF);
    }
    
    int text_y_start = b.y + tool_h;
    int line_h = 16;
    int start_line = 0;
    int visible_lines = (b.height - tool_h) / line_h;
    
    for (int i = 0; i < visible_lines && (start_line + i) < main_editor.line_count; i++) {
        int ly = text_y_start + i * line_h + 2;
        char *line = main_editor.lines[start_line + i];
        if (line[0] != '\0') draw_text_sf_mono(line, b.x + 5, ly, 0xFF000000);
        if (start_line + i == main_editor.current_line && main_editor.is_open) { 
            int cursor_px = b.x + 5 + (main_editor.cursor_x * 9); 
            draw_rect_filled((rect_t){cursor_px, ly, 2, 14}, 0x000000);
        }
    }
    // Draw Save Button
    rect_t buf_r = {b.x + b.width - 60, b.y + 5, 50, 22};
    draw_rect_filled(buf_r, 0xFF404040); // Dark grey button
    draw_text("SAVE", buf_r.x + 10, buf_r.y + 6, 0xFFFFFFFF, 0);
}


static void editor_handle_event(gui_element_t *element, gui_event_t *event) {
    if (event->type == GUI_EVENT_MOUSE_DOWN) {
        // Check Save Button click
        rect_t b = element->bounds;
        rect_t save_btn = {b.x + b.width - 60, b.y + 5, 50, 22};
        if (point_in_rect(event->mouse.pos, save_btn)) {
            // Trigger Save
            if (strlen(main_editor.filename) == 0) {
                 // Prompt for name? Quick hack: default to "untitled.c"
                 strcpy(main_editor.filename, "untitled.c");
            }
            editor_save_file(&main_editor);
            gui_mgr.needs_redraw = 1;
            return;
        }
        
        // Handle cursor placement
        if (event->mouse.pos.y > b.y + 32) {
             // ... click to move cursor logic (omitted for brevity)
        }
    }
    
    // Key press logic
    if (event->type == GUI_EVENT_KEY_PRESS) {
// ...
         char key = event->keyboard.key;
         if (key == 27) { main_editor.mode = MODE_NORMAL; main_editor.cmd_len = 0; gui_mgr.needs_redraw = 1; return; }
         
         if (main_editor.mode == MODE_NORMAL) {
             if (key == 'i') main_editor.mode = MODE_INSERT;
             else if (key == ':') { main_editor.mode = MODE_COMMAND; main_editor.cmd_len = 0; main_editor.command_buffer[0] = ':'; main_editor.cmd_len = 1; main_editor.command_buffer[1] = 0; }
             // Navigation keys h,j,k,l
             else if (key == 'h') editor_move_cursor(&main_editor, -1, 0);
             else if (key == 'j') editor_move_cursor(&main_editor, 0, 1);
             else if (key == 'k') editor_move_cursor(&main_editor, 0, -1);
             else if (key == 'l') editor_move_cursor(&main_editor, 1, 0);
             else if (key == 'x') editor_delete_char(&main_editor);
             gui_mgr.needs_redraw = 1;
         }
         else if (main_editor.mode == MODE_INSERT) {
             if (key >= 32 && key <= 126) editor_insert_char(&main_editor, key);
             else if (key == '\n' || key == 13) editor_new_line(&main_editor);
             else if (key == '\b' || key == 8) editor_delete_char(&main_editor);
             gui_mgr.needs_redraw = 1;
         }
         else if (main_editor.mode == MODE_COMMAND) {
             if (key == '\n' || key == 13) {
                 // :w <name>
                 if (main_editor.command_buffer[1] == 'w') {
                     if (main_editor.command_buffer[2] == ' ') {
                         // Save As
                         char *name = &main_editor.command_buffer[3];
                         while(*name == ' ') name++;
                         if (*name) {
                             strcpy(main_editor.filename, name);
                             editor_save_file(&main_editor);
                         }
                     } else if (main_editor.command_buffer[2] == 0) {
                         editor_save_file(&main_editor);
                     } else if (main_editor.command_buffer[2] == 'q') {
                         editor_save_file(&main_editor);
                         main_editor.is_open = 0;
                     }
                 } else if (main_editor.command_buffer[1] == 'q') {
                     main_editor.is_open = 0;
                 }
                 main_editor.mode = MODE_NORMAL;
                 main_editor.cmd_len = 0;
                 gui_mgr.needs_redraw = 1;
             }
             else if (key == '\b' || key == 8) { if (main_editor.cmd_len > 0) main_editor.cmd_len--; main_editor.command_buffer[main_editor.cmd_len] = 0; if(main_editor.cmd_len==0) main_editor.mode=MODE_NORMAL; gui_mgr.needs_redraw = 1; }
             else if (key >= 32 && key <= 126) { if (main_editor.cmd_len < 63) { main_editor.command_buffer[main_editor.cmd_len++] = key; main_editor.command_buffer[main_editor.cmd_len] = 0; gui_mgr.needs_redraw = 1; } }
         }
    }
}
