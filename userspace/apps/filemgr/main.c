#include <stdlib.h>

// Icons (Assumed to be in same directory or include path)
// We need to manage headers. For now, we will create a helper header to include them all.
// But first, let's just include them if the build system finds them.
// Since we copied them to icon/*.h, we can change include paths.
// Userspace app compilation will use -Iuserspace/libc -Iuserspace/apps/filemgr/icons

#include "kora_folder.h"
#include "kora_file.h"
#include "kora_home.h"
#include "kora_desktop.h"
#include "kora_documents.h"
#include "kora_downloads.h"
#include "kora_music.h"
#include "kora_pictures.h"
#include "kora_videos.h"


// Helper string functions
int strcmp(const char *s1, const char *s2) {
    while(*s1 && (*s1 == *s2)) { s1++; s2++; }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

int strlen(const char *s) {
    int l=0; while(s[l]) l++; return l;
}

char *strcpy(char *dest, const char *src) {
    char *d = dest;
    while ((*d++ = *src++));
    return dest;
}

char *strncpy(char *dest, const char *src, int n) {
    char *d = dest;
    while (n > 0 && *src) {
        *d++ = *src++;
        n--;
    }
    while (n > 0) {
        *d++ = 0;
        n--;
    }
    return dest;
}

char *strcat(char *dest, const char *src) {
    char *d = dest;
    while (*d) d++;
    while ((*d++ = *src++));
    return dest;
}

char* strstr(const char *haystack, const char *needle) {
    if (!*needle) return (char*)haystack;
    for (; *haystack; haystack++) {
        const char *h = haystack;
        const char *n = needle;
        while (*h && *n && *h == *n) { h++; n++; }
        if (!*n) return (char*)haystack;
    }
    return 0;
}

char* fm_strchr(const char *s, int c) {
    while (*s != (char)c) {
        if (!*s++) return 0;
    }
    return (char *)s;
}

void *memset(void *s, int c, int n) {
    unsigned char *p = (unsigned char *)s;
    while(n--) *p++ = (unsigned char)c;
    return s;
}

// Colors
#define FM_BG_COLOR 0xFFFFFFFF
#define FM_SIDEBAR_COLOR 0xFFF3F3F3
#define FM_HEADER_COLOR 0xFFF9F9F9
#define FM_SELECTION_COLOR 0xFFCCE8FF
#define FM_TEXT_COLOR 0xFF202020

// Types
typedef struct {
    char name[128];
    int type; // 1=File, 2=Dir
} fm_entry_t;

// State
typedef struct {
    char current_path[256];
    int selected_index;
    fm_entry_t entry_cache[64];
    int entry_count;
    
    // UI Layout
    int width, height;
} fm_state_t;

static fm_state_t state;

// Prototypes
void refresh(void);

void init_fm() {
    memset(&state, 0, sizeof(state));
    strcpy(state.current_path, "/home/aakash");
    state.selected_index = -1;
    state.width = 800;
    state.height = 550;
    refresh();
}

void open_dir(const char *path) {
    if (path[0] == '/') strcpy(state.current_path, path);
    refresh();
}

void navigate_up() {
    if (strcmp(state.current_path, "/") == 0) return;
    char *last = 0; 
    char *p = state.current_path;
    while(*p) { if(*p == '/') last = p; p++; }
    
    if (last && last != state.current_path) {
        *last = 0;
    } else {
        strcpy(state.current_path, "/");
    }
    refresh();
}

void refresh() {
    print("[FM] Refresh: "); print(state.current_path); print("\n");
    
    state.entry_count = 0;
    int fd = open(state.current_path, 0); // Open dir
    if (fd < 0) {
        print("[FM] Failed to open dir\n");
        // Fallback to root
        if(strcmp(state.current_path, "/")!=0) {
            strcpy(state.current_path, "/");
            refresh();
        }
        return;
    }
    
    state.selected_index = -1;
    
    while(1) {
        dirent_t *d = readdir(fd);
        if (!d) break;
        if (state.entry_count >= 64) break;
        
        strncpy(state.entry_cache[state.entry_count].name, d->name, 127);
        
        // Determine type? readdir in libc lacks type. 
        // We can just try to open generic or assume file unless implicit logic.
        // Actually, let's assume unknown and stat later?
        // Or syscall `readdir` populates type? 
        // Our kernel readdir returns `struct dirent`.
        // Let's assume everything is a file unless we check?
        // In kernel we used `finddir_fs` -> checks flags.
        // In userspace, we have `stat` missing.
        // Let's assume directory if no extension for now, or use `open` with O_DIRECTORY check?
        // Better: Hack. Assume Directories are: bin, usr, dev, etc.
        // Proper way: Implement `stat`.
        // Hack for V1: Try to `open` it. If it succeeds, it's file. If it fails with EISDIR?
        // No, we can just use heuristics.
        
        // Simple heuristic: No extension = Folder?
        // But `ccl` has no extension. 
        // Actually, our readdir syscall implementation in kernel copies `d->ino`.
        // We don't have type in dirent_t struct in libc.
        // We should add type to dirent_t.
        // For now, assume everything is file unless known folder name.
        
        int type = 1; // File
        // Hacky Folder Detection logic for typical Mithl structure
        char *n = state.entry_cache[state.entry_count].name;
        if (!fm_strchr(n, '.')) {
             // Likely folder
             type = 2; // Dir
        }
        // Specific overrides
        if (strcmp(n, "ccl")==0) type=1;
        if (strcmp(n, "hello_linux")==0) type=1;
        
        state.entry_cache[state.entry_count].type = type;
        
        state.entry_count++;
    }
    close(fd);
}

// Drawing Handles
void draw_sidebar_item(int x, int y, const uint32_t* icon, const char* label, const char* target) {
    int active = (strcmp(state.current_path, target) == 0);
    if(active) draw_rect(x-10, y-5, 180, 38, FM_SELECTION_COLOR);
    draw_image(icon, x, y, 32, 32);
    draw_text(label, x+40, y+8, FM_TEXT_COLOR);
}

void render() {
    int w = state.width;
    int h = state.height;
    
    // Clear
    // draw_rect(0, 0, w, h, FM_BG_COLOR); // handled by panel logic usually? No, we draw full window client area.
    
    // Header
    int header_h = 50;
    draw_rect(0, 0, w, header_h, FM_HEADER_COLOR);
    draw_rect(0, header_h, w, 1, 0xFFE5E5E5); // Line
    
    // Address Bar
    draw_text(state.current_path, 80, 15, 0xFF444444);
    
    // Nav Buttons
    draw_rect(10, 12, 26, 26, 0xFFCCCCCC);
    draw_text("<", 18, 17, 0);
    draw_rect(42, 12, 26, 26, 0xFFCCCCCC);
    draw_text("^", 50, 17, 0);
    
    // Sidebar
    int sb_w = 200;
    draw_rect(0, header_h+1, sb_w, h-header_h-25, FM_SIDEBAR_COLOR);
    draw_rect(sb_w, header_h+1, 1, h-header_h-25, 0xFFE5E5E5);
    
    int sb_x = 16;
    int sb_y = header_h + 16;
    draw_sidebar_item(sb_x, sb_y, kora_home, "Home", "/user/home"); sb_y+=42;
    draw_sidebar_item(sb_x, sb_y, kora_desktop, "Desktop", "/home/aakash/Desktop"); sb_y+=42;
    draw_sidebar_item(sb_x, sb_y, kora_documents, "Documents", "/home/aakash/Documents"); sb_y+=42;
    // ... others skipped for brevity
    
    // Grid
    int main_x = sb_w + 1;
    int main_w = w - main_x;
    int main_y = header_h + 1;
    
    // White BG for grid
    draw_rect(main_x, main_y, main_w, h-main_y-25, 0xFFFFFFFF);
    
    int item_w = 90;
    int item_h = 90;
    int cols = (main_w - 20) / item_w;
    if (cols < 1) cols = 1;
    
    for(int i=0; i<state.entry_count; i++) {
        int r = i/cols;
        int c = i%cols;
        int x = main_x + 20 + c*item_w;
        int y = main_y + 20 + r*item_h;
        
        if (i == state.selected_index) {
             draw_rect(x-5, y-5, item_w-10, item_h-10, FM_SELECTION_COLOR);
        }
        
        const uint32_t *icon = kora_file;
        if (state.entry_cache[i].type == 2) icon = kora_folder;
        // else check extensions
        
        draw_image(icon, x+24, y+10, 32, 32);
        
        int name_len = strlen(state.entry_cache[i].name);
        draw_text(state.entry_cache[i].name, x + (80 - name_len*9)/2, y+50, FM_TEXT_COLOR);
    }
    
    // Status
    int status_y = h - 24;
    draw_rect(0, status_y, w, 24, 0xFFF9F9F9);
    draw_text("Items:", 10, status_y+6, 0xFF606060);
    // Print count
    char buf[10]; int n=state.entry_count; int p=0; 
    if(n==0) buf[p++]='0';
    else {
        char t[10]; int ti=0;
        while(n>0) { t[ti++]=(n%10)+'0'; n/=10; }
        while(ti>0) buf[p++] = t[--ti];
    }
    buf[p]=0;
    draw_text(buf, 60, status_y+6, 0xFF606060);
}

void on_click(int mx, int my) {
    // Check Header
    if (my < 50) {
        if (mx > 10 && mx < 36) navigate_up(); // Back
        if (mx > 42 && mx < 68) navigate_up(); // Up
        return;
    }
    
    int sb_w = 200;
    if (mx < sb_w) {
        // Sidebar logic
        if (my > 50+16 && my < 50+16+32) open_dir("/user/home");
        else if (my > 50+16+42 && my < 50+16+42+32) open_dir("/home/aakash/Desktop");
        else if (my > 50+16+84 && my < 50+16+84+32) open_dir("/home/aakash/Documents");
        return;
    }
    
    // Grid
    int grid_x = sb_w;
    int grid_y = 50;
    int item_w = 90;
    int item_h = 90;
    int grid_w = state.width - sb_w;
    int cols = (grid_w - 20) / item_w;
    
    int rx = mx - grid_x - 20;
    int ry = my - grid_y - 20;
    
    if (rx < 0 || ry < 0) return;
    
    int c = rx / item_w;
    int r = ry / item_h;
    
    int idx = r * cols + c;
    
    if (idx < state.entry_count) {
        if (idx == state.selected_index) {
            // Double Click Open
            char path[256];
            strcpy(path, state.current_path);
            if(path[strlen(path)-1]!='/') strcat(path, "/");
            strcat(path, state.entry_cache[idx].name);
            
            if (state.entry_cache[idx].type == 2) {
                open_dir(path);
            } else {
                // Execute or edit
                int pid = fork();
                if (pid == 0) {
                    char *args[] = {path, 0};
                    char *env[] = {0};
                    if (execve(path, args, env) < 0) {
                         // Likely text file, run notepad
                         char *nargs[] = {"/bin/notepad.elf", path, 0};
                         execve("/bin/notepad.elf", nargs, env);
                         exit(1);
                    }
                }
            }
        } else {
            state.selected_index = idx;
        }
    } else {
        state.selected_index = -1;
    }
}

int main() {
    create_window("File Manager", 80, 80, 800, 550);
    init_fm();
    
    while(1) {
        render(); // Should be event driven, but for V1 we redraw always? No, wait for event.
        
        gui_event_t ev;
        if (get_event(&ev)) {
            if (ev.type == GUI_EVENT_WINDOW_CLOSE) {
                exit(0);
            }
            if (ev.type == GUI_EVENT_MOUSE_DOWN) {
                 on_click(ev.mouse.pos.x, ev.mouse.pos.y);
            }
            if (ev.type == GUI_EVENT_KEY_PRESS) {
                if (ev.keyboard.key == '\n') {
                    // Open selected
                }
            }
        }
    }
    return 0;
}
