#ifndef STDLIB_H
#define STDLIB_H

// Basic Types
typedef unsigned int uint32_t;
typedef int int32_t;
typedef unsigned short uint16_t;
typedef unsigned char uint8_t;

// Process
void exit(int status);
void print(const char *msg);

// Helpers
int get_cmdline(char *buf, int max_len);

// GUI Types
typedef struct { int x, y; } point_t;
typedef struct { int x, y, width, height; } rect_t;
typedef enum {
    GUI_EVENT_MOUSE_DOWN, GUI_EVENT_MOUSE_UP, GUI_EVENT_MOUSE_MOVE, GUI_EVENT_MOUSE_SCROLL,
    GUI_EVENT_KEY_PRESS, GUI_EVENT_WINDOW_CLOSE
} gui_event_type_t;
typedef struct {
    gui_event_type_t type;
    union {
        struct { point_t pos; int button; int scroll_delta; } mouse;
        struct { char key; uint8_t modifiers; uint8_t raw_code; } keyboard;
    };
} gui_event_t;

// GUI Functions
int create_window(const char *title, int x, int y, int w, int h);
int get_event(gui_event_t *event);
void draw_rect(int x, int y, int w, int h, uint32_t color);
void draw_text(const char *msg, int x, int y, uint32_t color);

// File System
typedef struct {
    char name[128];
    uint32_t ino;
} dirent_t;
// Stdio
void print(const char *s);
void gets(char *buf);

// Process
int fork();
int waitpid(int pid, int *status, int options);
char getchar();

// File Operations
int open(const char *pathname, int flags);
int close(int fd);
int read(int fd, void *buf, int count);
int write(int fd, const void *buf, int count);
dirent_t *readdir(int fd);
int mkdir(const char *path, uint32_t mode);
int unlink(const char *pathname);
int rename(const char *oldpath, const char *newpath);
int unlink(const char *pathname);
int rename(const char *oldpath, const char *newpath);
int creat(const char *pathname, uint32_t mode);

// Exec
int execve(const char *filename, char *const argv[], char *const envp[]);

// Process List
typedef struct {
    int pid;
    char name[32];
    int state;
} process_info_t;

int get_process_list(process_info_t *buf, int max_count);

#endif
