#include "terminal.h"
#include "commands.h"
#include "desktop.h"
#include "memory.h"
#include "memory.h"
#include "string.h"
#include "graphics.h"
#include "vfs.h"
#include "process.h"

#include <theme.h>

extern const uint8_t scancode_to_ascii[128]; // From keyboard.c

static void terminal_draw_content(gui_renderer_t *renderer, gui_element_t *element);
static void terminal_handle_event(gui_element_t *element, gui_event_t *event);

terminal_t *terminal_create(void)
{
    terminal_t *term = (terminal_t *)memory_alloc(sizeof(terminal_t));
    if (!term) return NULL;

    // Create a window for the terminal
    // Adjusted size for 11px char width (80*11=880) + padding
    term->window = gui_create_window("Terminal", 100, 100, 920, 520); 
    if (!term->window) {
        memory_free(term);
        return NULL;
    }
    
    // Create a panel for the terminal content
    term->panel = gui_create_panel(0, 0, 920, 520 - 48); // Initial bounds
    if (!term->panel) {
        memory_free(term);
        return NULL;
    }
                     
    // Set up terminal state
                     
    // Set up terminal state
    term->cursor_x = 0;
    term->cursor_y = 0;
    
    // Use Theme Colors
    theme_t *theme = theme_get_current();
    term->panel->base.background_color = theme->terminal_bg;
    term->bg_color = theme->terminal_bg;
    term->fg_color = theme->terminal_fg;

    term->input_len = 0;
    memset(term->buffer, 0, sizeof(term->buffer));
    memset(term->input_buffer, 0, sizeof(term->input_buffer));
    
    // Override PANEL content drawing and event handling
    term->panel->base.draw = terminal_draw_content; 
    term->panel->base.event_handler = terminal_handle_event;
    
    // Add "Terminal" tab
    gui_window_add_tab(term->window, "Terminal", (gui_element_t*)term->panel);
    
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
         // Reset position if it was hidden/closed (x < 0)
         if (active_term->window->base.bounds.x < 0) {
             gui_set_position((gui_element_t*)active_term->window, 100, 100);
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

void terminal_print(terminal_t *term, const char *str) {
    while (*str) {
        terminal_put_char(term, *str++);
    }
}

void terminal_active_write(const char *buf, uint32_t len) {
    if (active_term) {
        for(uint32_t i=0; i<len; i++) {
            terminal_put_char(active_term, buf[i]);
        }
    }
}

// Rust FFI
extern size_t rust_handle_command(const char *input, size_t len, char *output, size_t max_out);

// Prompt Logic
extern const char* get_current_dir(void);

static void terminal_print_prompt(terminal_t *term) {
    // Print user@host in Green (if we supported colors, but we only have FG)
    // Wait, terminal buffer is just chars. 
    // Limitation: This terminal doesn't store color attributes per char.
    // Making it fully colored like the screenshot requires major refactor (struct term_char { char c; uint32_t color; }).
    // User wants "clean as like in the second image".
    // Second image has Green prompt.
    // I CANNOT do that without refactoring terminal buffer structure.
    // Given constraints, I will stick to White text for now, but maybe implemented hacky "last line color override"? No.
    // I'll stick to full clean White. It's clean.
    
    terminal_print(term, "aakash@mithl:");
    
    const char *cwd = get_current_dir();
    
    // Check if starts with /home/aakash
    if (memcmp(cwd, "/home/aakash", 12) == 0) {
        terminal_print(term, "~");
        terminal_print(term, cwd + 12);
    } else {
        terminal_print(term, cwd);
    }
    
    terminal_print(term, "$ ");
}

// static const char *PROMPT = "aakash@mithl:~$ "; // REMOVED

void terminal_run_command(terminal_t *term, const char *command) {
    terminal_print(term, "\n");
    
    if (strlen(command) == 0) {
        terminal_print_prompt(term);
        return;
    }

    // Special handling for clear
    if (memcmp(command, "clear", 5) == 0) {
        memset(term->buffer, 0, sizeof(term->buffer));
        term->cursor_x = 0;
        term->cursor_y = 0;
        terminal_print_prompt(term);
        return;
    }
    
    // Try new command system first
    extern void execute_command(terminal_t *term, const char *cmdline);
    extern const command_t* get_commands(void);
    
    const command_t *cmds = get_commands();
    int found = 0;
    
    // Parse command name
    char cmd_name[64];
    int i = 0;
    while (command[i] && command[i] != ' ' && i < 63) {
        cmd_name[i] = command[i];
        i++;
    }
    cmd_name[i] = 0;
    
    // Check if it's in new command system
    for (int j = 0; cmds[j].name; j++) {
        if (strcmp(cmd_name, cmds[j].name) == 0) {
            execute_command(term, command);
            found = 1;
            break;
        }
    }
    
    if (found) {
        terminal_print_prompt(term);
        return;
    }
    
    // Built-in Commands using VFS (legacy)
    // 1. Built-in Commands
    if (memcmp(command, "help", 4) == 0) {
        terminal_print(term, "Mithl-OS Advanced Terminal\n");
        terminal_print(term, "==========================\n");
        terminal_print(term, "Built-in Commands:\n");
        terminal_print(term, "  help      Show this help message.\n");
        terminal_print(term, "  clear     Clear the terminal screen.\n");
        terminal_print(term, "  cd <dir>  Change directory (e.g., cd /home).\n");
        terminal_print(term, "  pwd       Print working directory.\n");
        terminal_print(term, "  whoami    Show current user.\n");
        terminal_print(term, "  date      Show system date/time.\n");
        terminal_print(term, "  exit      Close the terminal.\n");
        terminal_print(term, "\n");
        terminal_print(term, "System Utilities (Userspace):\n");
        terminal_print(term, "  ls        List directory contents.\n");
        terminal_print(term, "  ps        List running processes.\n");
        terminal_print(term, "  cat <f>   Display file contents.\n");
        terminal_print(term, "\n");
        terminal_print(term, "Applications:\n");
        terminal_print(term, "  hello     Running 'Hello World' GUI App.\n");
        terminal_print(term, "  calc      Launch Calculator.\n");
    }
    else if (memcmp(command, "ls", 2) == 0) {
        // Try to execute /ls.elf
        extern fs_node_t *finddir_fs(fs_node_t *node, char *name);
        extern fs_node_t *fs_root;
        
        // Fast path: check if /ls.elf exists
        if (finddir_fs(fs_root, "ls.elf")) {
             process_create_elf("ls", "/ls.elf", "");
             // Note: It runs async. Prompt appears immediately.
             // Ideal: wait for process.
        } else {
             terminal_print(term, "ls: Command not found or /ls.elf missing.\n");
        }
    }
    else if (memcmp(command, "cat ", 4) == 0) {
        // Extract arguments
        char *arg = (char*)command + 4;
        while(*arg == ' ') arg++;
        
         if (finddir_fs(fs_root, "cat.elf")) {
             process_create_elf("cat", "/cat.elf", arg);
         } else {
             terminal_print(term, "cat: /cat.elf not found.\n");
         }
    }
    else if (memcmp(command, "ps", 2) == 0) {
        extern fs_node_t *finddir_fs(fs_node_t *node, char *name);
        extern fs_node_t *fs_root;
        if (finddir_fs(fs_root, "ps.elf")) {
             process_create_elf("ps", "/ps.elf", "");
        } else {
             terminal_print(term, "ps: Command not found or /ps.elf missing.\n");
        }
    }
    else if (memcmp(command, "calc", 4) == 0) {
         process_create_elf("Calculator", "/calculator.elf", "");
    }
    else if (memcmp(command, "hello", 5) == 0) {
         process_create_elf("Hello", "/hello.elf", "");
    }
    else if (memcmp(command, "mkdir ", 6) == 0) {
         char *arg = (char*)command + 6;
         while(*arg == ' ') arg++;
         
         if (finddir_fs(fs_root, "mkdir.elf")) {
             process_create_elf("mkdir", "/mkdir.elf", arg);
         } else {
             terminal_print(term, "mkdir: /mkdir.elf not found.\n");
         }
    }
    else if (memcmp(command, "cp ", 3) == 0) {
         char *arg = (char*)command + 3;
         while(*arg == ' ') arg++;
         
         if (finddir_fs(fs_root, "cp.elf")) {
             process_create_elf("cp", "/cp.elf", arg);
         } else {
             terminal_print(term, "cp: /cp.elf not found.\n");
         }
    }
    else if (memcmp(command, "mv ", 3) == 0) {
         char *arg = (char*)command + 3;
         while(*arg == ' ') arg++;
         
         if (finddir_fs(fs_root, "mv.elf")) {
             process_create_elf("mv", "/mv.elf", arg);
         } else {
             terminal_print(term, "mv: /mv.elf not found.\n");
         }
    }
    else if (memcmp(command, "cc ", 3) == 0) {
         char *arg = (char*)command + 3;
         while(*arg == ' ') arg++;
         
         if (finddir_fs(fs_root, "cc.elf")) {
             process_create_elf("cc", "/cc.elf", arg);
         } else {
             terminal_print(term, "cc: /cc.elf not found.\n");
         }
    }
    else if (memcmp(command, "clear", 5) == 0) {
        // Handled above usually, but fallback
        memset(term->buffer, 0, sizeof(term->buffer));
        term->cursor_x = 0; term->cursor_y = 0;
    }
    else if (memcmp(command, "pwd", 3) == 0) {
        terminal_print(term, "/\n");
    }
    else if (memcmp(command, "date", 4) == 0) {
        terminal_print(term, "Current Time: TBD (RTC not linked)\n");
    }
    // Generic ELF Launcher (e.g. ./program.elf)
    else if (command[0] == '/' && strlen(command) > 4) {
         process_create_elf("UserApp", command, "");
    }
    else {
        terminal_print(term, "Unknown command: ");
        terminal_print(term, command);
        terminal_print(term, "\n");
    }
    
    terminal_print_prompt(term);
}

void terminal_run_command_active(const char *command) {
    if (active_term) {
        terminal_run_command(active_term, command);
    }
}

static void terminal_draw_content(gui_renderer_t *renderer, gui_element_t *element) {
    (void)renderer; // We use direct graphics calls for custom font control
    
    // 0. VISIBILITY CHECK
    // If the main window is hidden (x = -1000), DO NOT DRAW the panel.
    // This fixes the "Ghosting" issue where the panel stays on screen after closing.
    if (active_term && active_term->window) {
        if (active_term->window->base.bounds.x < -500) return;
        
        // Also, sync panel bounds with window for dragging
        // The panel should be at window.x, window.y + title_h
        // This fixes "Terminal acts as separate window" (Detached content)
        int title_h = 30;
        rect_t *w_r = &active_term->window->base.bounds;
        element->bounds.x = w_r->x;
        element->bounds.y = w_r->y + title_h;
        element->bounds.width = w_r->width;
        element->bounds.height = w_r->height - title_h;
    }

    // 1. Draw Background
    draw_rect_filled(element->bounds, active_term->bg_color);
    
    // Bounds check
    rect_t bounds = element->bounds;
    
    int content_y = bounds.y + 5;
    int content_x = bounds.x + 5;
    
    // Font Constants
    int line_height = 18; // SF is 16px high + padding
    int char_width = 11;  // Fixed cell width for mono emulation
    
    if (active_term && active_term->window) { 
        for (int row = 0; row < TERM_ROWS; row++) {
            // Draw using SF Mono Helper directly from graphics.h
            // renderer->draw_text usually points to generic draw_text.
            // We want specific mono behavior.
            
            int y = content_y + row * line_height;
            if (y + line_height > bounds.y + bounds.height) break; 
            
            char *line = active_term->buffer[row];
            draw_text_sf_mono(line, content_x, y, active_term->fg_color);
        }
        
        // Draw Cursor (Block)
        if (active_term->cursor_y < TERM_ROWS) {
             rect_t cursor_rect = {
                 content_x + active_term->cursor_x * char_width,
                 content_y + active_term->cursor_y * line_height,
                 char_width, 
                 line_height
             };
             draw_rect_filled(cursor_rect, 0xAAFFFFFF); // Semi-transparent White Block
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
        char ascii = event->keyboard.key;
        // uint8_t scancode = event->keyboard.raw_code;
        
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

