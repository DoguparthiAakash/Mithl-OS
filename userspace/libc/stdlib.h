#ifndef STDLIB_H
#define STDLIB_H

void exit(int status);
void print(const char *msg);
int create_window(const char *title, int x, int y, int w, int h);

// Basic types
typedef unsigned int uint32_t;
typedef int int32_t;
typedef unsigned short uint16_t;
typedef unsigned char uint8_t;

typedef struct {
    int x, y;
} point_t;

typedef struct {
    int x, y, width, height;
} rect_t;

typedef enum {
    GUI_EVENT_MOUSE_DOWN,
    GUI_EVENT_MOUSE_UP,
    GUI_EVENT_MOUSE_MOVE,
    GUI_EVENT_MOUSE_SCROLL,
    GUI_EVENT_KEY_PRESS,
    GUI_EVENT_WINDOW_CLOSE
} gui_event_type_t;

typedef struct {
    gui_event_type_t type;
    union {
        struct {
            point_t pos;
            int button;
            int scroll_delta;
        } mouse;
        struct {
            char key;
            uint8_t modifiers;
            uint8_t raw_code;
        } keyboard;
    };
} gui_event_t;

void exit(int status);
void print(const char *msg);
int create_window(const char *title, int x, int y, int w, int h);
int get_event(gui_event_t *event);
void draw_rect(int x, int y, int w, int h, uint32_t color);
void draw_text(const char *msg, int x, int y, uint32_t color);

#endif
