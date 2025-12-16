#include "terminal.h"
#include "desktop.h"
#include "memory.h"
#include "memory.h"
#include "string.h"
#include "graphics.h"
#include "vfs.h"

extern const uint8_t scancode_to_ascii[128]; // From keyboard.c

static void terminal_draw_content(gui_renderer_t *renderer, gui_element_t *element);
static void terminal_handle_event(gui_element_t *element, gui_event_t *event);

terminal_t *terminal_create(void)
{
    terminal_t *term = (terminal_t *)memory_alloc(sizeof(terminal_t));
    if (!term) return NULL;

    // Create a window for the terminal
    term->window = gui_create_window("Terminal", 100, 100, 640, 400); // 80 cols * 8px = 640, 25 rows * 16px approx
    if (!term->window) {
        memory_free(term);
        return NULL;
    }
    
    // Create a panel for the terminal content
    term->panel = gui_create_panel(0, 0, 640, 400 - 48); // Initial bounds
    if (!term->panel) {
        memory_free(term);
        return NULL;
    }
                     
    // Set up terminal state
                     
    // Set up terminal state
    term->cursor_x = 0;
    term->cursor_y = 0;
    term->bg_color = 0xFF000000; // Black (Opaque)
    term->panel->base.background_color = 0xFF000000; 
    term->fg_color = 0xFF00FF00; // Green
    term->input_len = 0;
    memset(term->buffer, 0, sizeof(term->buffer));
    memset(term->input_buffer, 0, sizeof(term->input_buffer));
    
    // Override PANEL content drawing and event handling
    term->panel->base.draw = terminal_draw_content; 
    term->panel->base.event_handler = terminal_handle_event;
    
    // Add "Terminal" tab
    gui_window_add_tab(term->window, "Terminal", (gui_element_t*)term->panel);
    
    // Link terminal struct to element (hacky user_data)
    // Ideally gui_element_t should have void* user_data. 
    // Assuming for now we can infer or use a global if single terminal.
    // Let's use a global for simplicity for this demo, or add user_data to gui_element_t.
    // Since I can't easily change gui.h struct safely repeatedly without breaking ABI if objects compiled, 
    // I'll use a static single instance pointer for this demo.
    
    return term;
}

static terminal_t *active_term = NULL; // Single active terminal for demo

void terminal_init(void) {
    if (active_term) return; // Already initialized

    active_term = terminal_create();
    if(active_term && active_term->window) {
        // Add to GUI root
        if (gui_mgr.root) {
            gui_add_element(gui_mgr.root, (gui_element_t*)active_term->window);
        } else {
             // If root is missing, we can't show it. Log error?
             return;
        }
        
        // Print welcome message
        strcpy(active_term->buffer[0], "Mithl OS Terminal v1.1");
        strcpy(active_term->buffer[1], "Type 'help' for commands.");
        active_term->cursor_y = 2;
        strcpy(active_term->buffer[2], "aakash@mithl:~$ ");
        active_term->cursor_x = 16; // Length of prompt
    }
}

// Helper to spawn or show terminal
void terminal_show(void) {
    if (!active_term) {
        terminal_init();
    }
    
    // Check again
    if (active_term && active_term->window) {
         // Reset position if it was hidden/closed
         if (active_term->window->base.bounds.x == -9999) {
             active_term->window->base.bounds.x = 100;
             active_term->window->base.bounds.y = 100;
         }
         
         // Bring to front (Conceptually)
         gui_mgr.focused_element = (gui_element_t*)active_term->window;
         gui_mgr.needs_redraw = 1;
    }
}

static void terminal_scroll(terminal_t *term) {
    if (term->cursor_y >= TERM_ROWS) {
        // Move lines up
        for (int i = 0; i < TERM_ROWS - 1; i++) {
            memcpy(term->buffer[i], term->buffer[i+1], TERM_COLS);
        }
        memset(term->buffer[TERM_ROWS-1], 0, TERM_COLS);
        term->cursor_y = TERM_ROWS - 1;
    }
}

static void terminal_put_char(terminal_t *term, char c) {
    if (c == '\n') {
        term->cursor_x = 0;
        term->cursor_y++;
        terminal_scroll(term);
    } else if (c == '\b') {
        if (term->cursor_x > 0) {
            term->cursor_x--;
            term->buffer[term->cursor_y][term->cursor_x] = 0;
        }
    } else {
        if (term->cursor_x < TERM_COLS) {
            term->buffer[term->cursor_y][term->cursor_x] = c;
            term->cursor_x++;
        }
    }
}

static void terminal_print(terminal_t *term, const char *str) {
    while (*str) {
        terminal_put_char(term, *str++);
    }
}

// Rust FFI
extern size_t rust_handle_command(const char *input, size_t len, char *output, size_t max_out);

// Prompt Constant
static const char *PROMPT = "aakash@mithl:~$ ";

void terminal_run_command(terminal_t *term, const char *command) {
    terminal_print(term, "\n");
    
    if (strlen(command) == 0) {
        terminal_print(term, PROMPT);
        return;
    }

    // Special handling for clear
    if (memcmp(command, "clear", 5) == 0) {
        memset(term->buffer, 0, sizeof(term->buffer));
        term->cursor_x = 0;
        term->cursor_y = 0;
        terminal_print(term, PROMPT);
        return;
    }
    
    // Built-in Commands using VFS
    if (memcmp(command, "ls", 2) == 0) {
        // List root or current directory
        extern fs_node_t *fs_root;
        fs_node_t *node = fs_root; // Should be current_dir ideally
        
        // Find /home/aakash if specific path support added later
        // For now, list root
        if (node) {
             struct dirent *d;
             int i = 0;
             while ((d = readdir_fs(node, i)) != 0) {
                 terminal_print(term, d->name);
                 terminal_print(term, "  ");
                 i++;
             }
             terminal_print(term, "\n");
        } else {
             terminal_print(term, "Filesystem error.\n");
        }
    }
    else if (memcmp(command, "cat ", 4) == 0) {
        // Simple cat implementation: cat filename
        char filename[64];
        char *arg = (char*)command + 4;
        while(*arg == ' ') arg++; // skip spaces
        strcpy(filename, arg);
        
        extern fs_node_t *fs_root;
        // Search in root for now (flat structure limitation of simple finddir usage unless we traverse)
        // Assume full path or just file in root?
        // Let's retry finding in /home/aakash for realism?
        // For simplicity: Search recursively or just hardcode checking common paths?
        
        fs_node_t *file = finddir_fs(fs_root, filename);
        if (!file) {
             // Check home/aakash
             fs_node_t *home = finddir_fs(fs_root, "home");
             if (home) {
                 fs_node_t *user = finddir_fs(home, "aakash");
                 if (user) file = finddir_fs(user, filename);
             }
        }
        
        if (file) {
             uint8_t buf[256];
             uint32_t sz = read_fs(file, 0, 255, buf);
             buf[sz] = 0;
             terminal_print(term, (char*)buf);
             terminal_print(term, "\n");
        } else {
             terminal_print(term, "File not found.\n");
        }
    }
    else if (memcmp(command, "pwd", 3) == 0) {
        terminal_print(term, "/\n");
    }
    else if (memcmp(command, "whoami", 6) == 0) {
        terminal_print(term, "aakash\n");
    }
    else if (memcmp(command, "date", 4) == 0) {
        terminal_print(term, "Sun Dec 14 15:30:00 IST 2025\n");
    }
    else if (memcmp(command, "help", 4) == 0) {
        terminal_print(term, "Available commands: ls, cat, pwd, whoami, date, clear, hello, snake, guess\n");
    }
    else if (memcmp(command, "hello", 5) == 0) {
        extern int hello_app(terminal_t *term);
        hello_app(term);
    }
    else if (memcmp(command, "snake", 5) == 0) {
        extern int snake_app(terminal_t *term);
        snake_app(term);
    }
    else if (memcmp(command, "guess", 5) == 0) {
        extern int guess_number_app(terminal_t *term);
        guess_number_app(term);
    }
    else {
        // Rust Shell or Unknown
        char output_buffer[256];
        memset(output_buffer, 0, sizeof(output_buffer));
        size_t len = strlen(command);
        extern size_t rust_handle_command(const char *input, size_t len, char *output, size_t max_out);
        size_t result_len = rust_handle_command(command, len, output_buffer, 255);
        
        if (result_len > 0) {
            terminal_print(term, output_buffer);
            terminal_print(term, "\n");
        } else {
             terminal_print(term, "Command not found: ");
             terminal_print(term, command);
             terminal_print(term, "\n");
        }
    }
    
    terminal_print(term, PROMPT);
}

void terminal_run_command_active(const char *command) {
    if (active_term) {
        terminal_run_command(active_term, command);
    }
}

static void terminal_draw_content(gui_renderer_t *renderer, gui_element_t *element) {
    // 1. Draw Background
    draw_rect_filled(element->bounds, active_term->bg_color);
    
    // Bounds check
    rect_t bounds = element->bounds;
    
    // Draw Content (Offset by Title Bar)
    // No Title bar offset usage
    // int title_height = 24; 
    
    int content_y = bounds.y + 5;
    int content_x = bounds.x + 5;
    
    if (active_term && active_term->window) { // Ensure term exists and we are drawing IT
        for (int row = 0; row < TERM_ROWS; row++) {
            point_t pos = {content_x, content_y + row * 12}; // 12px line height
            char *line = active_term->buffer[row];
            
            // Check for prompt pattern "aakash@mithl:~$ " or similar
            // Hacky detection: "aakash@mithl"
            char *prompt_start = strstr(line, "aakash@mithl");
            char *path_sep = strstr(line, ":");
            char *suffix = strstr(line, "$");
            
            if (prompt_start == line && path_sep && suffix && path_sep > prompt_start && suffix > path_sep) {
                // We have a prompt line!
                // 1. Draw "aakash@mithl" in GREEN
                int len_user = path_sep - prompt_start;
                char user_part[32];
                strncpy(user_part, line, len_user);
                user_part[len_user] = '\0';
                
                renderer->draw_text(user_part, pos, 0x00FF00); // Green
                
                // 2. Draw ":path" in BLUE
                // Calc offset
                pos.x += len_user * 8; 
                int len_path = (suffix - path_sep); // includes ':' but stops before '$'
                char path_part[64];
                strncpy(path_part, path_sep, len_path);
                path_part[len_path] = '\0';
                
                renderer->draw_text(path_part, pos, 0x0000FF); // Blue
                
                // 3. Draw "$ " and rest in WHITE
                pos.x += len_path * 8;
                renderer->draw_text(suffix, pos, 0xFFFFFF); // White
            } else {
                // Normal line
                renderer->draw_text(line, pos, active_term->fg_color); 
            }
        }
        
        // Draw Cursor (Block)
        if (active_term->cursor_y < TERM_ROWS) {
             point_t cursor_pos = {
                 content_x + active_term->cursor_x * 8,
                 content_y + active_term->cursor_y * 12
             };
             rect_t cursor_rect = {cursor_pos.x, cursor_pos.y, 8, 12};
             draw_rect_filled(cursor_rect, 0xFFFFFF); // White Block Cursor
        }
    }
}

static void terminal_handle_event(gui_element_t *element, gui_event_t *event) {
    if (!active_term) return;
    
    // 1. Base Window Handling (Drag, Close, etc)
    // 1. Panel Handling
    // gui_window_event_handler(element, event); // REMOVED: Window handles dragging
    
    // If the event caused a state change (like close), we might need to stop.
    // If window closed (x=-9999), stop processing.
    if (element->bounds.x == -9999) return;
    
    if (event->type == GUI_EVENT_KEY_PRESS) {
        char key = event->keyboard.key;
        uint8_t scancode = (uint8_t)key;
        char ascii = 0;
        if (scancode < 128) ascii = scancode_to_ascii[scancode];
        
        if (ascii) {
            if (ascii == '\n') { // Enter
                active_term->input_buffer[active_term->input_len] = 0;
                terminal_run_command(active_term, active_term->input_buffer);
                active_term->input_len = 0;
            } else if (ascii == '\b') { // Backspace
                if (active_term->input_len > 0) {
                    active_term->input_len--;
                    active_term->input_buffer[active_term->input_len] = 0;
                    terminal_put_char(active_term, '\b');
                }
            } else {
                if (active_term->input_len < 255) {
                    active_term->input_buffer[active_term->input_len++] = ascii;
                    terminal_put_char(active_term, ascii);
                }
            }
        }
    }
    // Remove custom dragging code (handled by gui_window_event_handler)
    // Keep Right Click/Scroll if desired
    else if (event->type == GUI_EVENT_MOUSE_UP && event->mouse.button == 2) {
         if (active_term->bg_color == 0x000000) {
             active_term->bg_color = 0x000080; // Dark Blue
         } else {
             active_term->bg_color = 0x000000; // Black
         }
         terminal_print(active_term, "\n[Right Click: Theme Toggled]\n> ");
    }
    else if (event->type == GUI_EVENT_MOUSE_SCROLL) {
         if (event->mouse.scroll_delta > 0) {
             terminal_print(active_term, "[Scroll Up]");
         } else if (event->mouse.scroll_delta < 0) {
             terminal_print(active_term, "[Scroll Down]");
         }
    }
}

