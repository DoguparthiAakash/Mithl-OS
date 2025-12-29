#ifndef STDLIB_H
#define STDLIB_H

#include "stdint.h"
#include <stddef.h>


// Basic Types (Removed, using stdint.h)

// Process
void exit(int status);
void print(const char *msg);

extern char **environ;

// Memory
void *malloc(size_t size);
void free(void *ptr);
void *calloc(size_t nmemb, size_t size);
void *realloc(void *ptr, size_t size);
int system(const char *command);
char *getenv(const char *name);
int atoi(const char *nptr);
char *realpath(const char *path, char *resolved_path);
char *getcwd(char *buf, size_t size);
float strtof(const char *nptr, char **endptr);

// Conversions
long strtol(const char *nptr, char **endptr, int base);
unsigned long strtoul(const char *nptr, char **endptr, int base);
long long strtoll(const char *nptr, char **endptr, int base);
unsigned long long strtoull(const char *nptr, char **endptr, int base);

// Alg
void qsort(void *base, size_t nmemb, size_t size, int (*compar)(const void *, const void *));

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
void draw_rect(int x, int y, int w, int h, uint32_t color);
void draw_text(const char *msg, int x, int y, uint32_t color);
void draw_image(const uint32_t *data, int x, int y, int w, int h);

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
int close(int fd);
int read(int fd, void *buf, uint32_t count);
int write(int fd, const void *buf, uint32_t count);
int pipe(int pipefd[2]);
int dup2(int oldfd, int newfd);
dirent_t *readdir(int fd);
int mkdir(const char *path, uint32_t mode);
int unlink(const char *pathname);
int rename(const char *oldpath, const char *newpath);
int unlink(const char *pathname);
int rename(const char *oldpath, const char *newpath);
int chdir(const char *path);
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

// Semantic Types
#define AGENT_OP_REGISTER 1
#define AGENT_OP_QUERY    2

typedef struct {
    char name[64];
    char intents[128];
    char binary[128];
    int  trust_level;
    int  active;
} agent_node_t;

int agent_op(int op, void *arg1, void *arg2);

#endif
