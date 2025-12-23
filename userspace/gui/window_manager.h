#ifndef WINDOW_MANAGER_H
#define WINDOW_MANAGER_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    int id;
    int x, y;
    int width, height;
    char title[256];
    uint32_t *pixels; // ARGB buffer
    bool visible;
    bool focused;
    bool needs_redraw;
} wm_window_t;

typedef struct {
    wm_window_t **windows;
    int count;
    int capacity;
    int focused_idx;
    
    // Screen dimensions
    int width;
    int height;
    
    // Background
    uint32_t background_color;
} wm_manager_t;

// Core API
wm_manager_t* wm_init(int width, int height);
void wm_destroy(wm_manager_t *wm);

// Window Management
wm_window_t* wm_create_window(wm_manager_t *wm, int x, int y, int width, int height, const char *title);
void wm_destroy_window(wm_manager_t *wm, wm_window_t *win);
void wm_focus_window(wm_manager_t *wm, wm_window_t *win);
void wm_move_window(wm_window_t *win, int x, int y);

// Rendering
// Composites all windows into the provided output buffer (stride = width * 4)
void wm_render(wm_manager_t *wm, uint32_t *output_buffer);

#endif
