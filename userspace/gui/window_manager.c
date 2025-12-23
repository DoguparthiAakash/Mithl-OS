#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "window_manager.h"

// Basic Color Constants (ARGB)
#define COLOR_BG        0xFF2E3440  // Nord Dark Grey
#define COLOR_WIN_BG    0xFFECEFF4  // Nord White
#define COLOR_TITLE_BG  0xFF4C566A  // Nord Grey
#define COLOR_TITLE_FG  0xFFD8DEE9  // Nord Light Grey
#define COLOR_BORDER    0xFF434C5E

wm_manager_t* wm_init(int width, int height) {
    wm_manager_t *wm = calloc(1, sizeof(wm_manager_t));
    if (!wm) return NULL;
    
    wm->width = width;
    wm->height = height;
    wm->capacity = 10;
    wm->windows = calloc(wm->capacity, sizeof(wm_window_t*));
    wm->background_color = COLOR_BG;
    
    return wm;
}

void wm_destroy(wm_manager_t *wm) {
    for (int i = 0; i < wm->count; i++) {
        wm_destroy_window(wm, wm->windows[i]);
    }
    free(wm->windows);
    free(wm);
}

wm_window_t* wm_create_window(wm_manager_t *wm, int x, int y, int width, int height, const char *title) {
    if (wm->count >= wm->capacity) return NULL; // Limit for now
    
    wm_window_t *win = calloc(1, sizeof(wm_window_t));
    win->x = x;
    win->y = y;
    win->width = width;
    win->height = height;
    strncpy(win->title, title, 255);
    win->visible = true;
    
    // Allocate pixel buffer
    win->pixels = calloc(width * height, sizeof(uint32_t));
    
    // Fill with default background
    for (int i = 0; i < width * height; i++) {
        win->pixels[i] = COLOR_WIN_BG;
    }
    
    // Attempt to be top
    wm->windows[wm->count++] = win;
    wm_focus_window(wm, win);
    
    return win;
}

void wm_destroy_window(wm_manager_t *wm, wm_window_t *win) {
    // Remove from list (simple swap remove)
    for (int i = 0; i < wm->count; i++) {
        if (wm->windows[i] == win) {
            // Shift elements down to keep order (for Z-order)
            for (int j = i; j < wm->count - 1; j++) {
                wm->windows[j] = wm->windows[j+1];
            }
            wm->count--;
            break;
        }
    }
    
    free(win->pixels);
    free(win);
}

void wm_focus_window(wm_manager_t *wm, wm_window_t *win) {
    // Bring to top (end of array)
    // First remove
    int found_idx = -1;
    for (int i = 0; i < wm->count; i++) {
        if (wm->windows[i] == win) {
            found_idx = i;
            break;
        }
    }
    
    if (found_idx == -1) return;
    
    // Unfocus all
    for(int i=0; i<wm->count; i++) wm->windows[i]->focused = false;
    
    // Shift others
    for (int j = found_idx; j < wm->count - 1; j++) {
        wm->windows[j] = wm->windows[j+1];
    }
    
    // Put at end
    wm->windows[wm->count - 1] = win;
    win->focused = true;
}

void wm_move_window(wm_window_t *win, int x, int y) {
    win->x = x;
    win->y = y;
}

// Simple software blit with clipping
static void blit_window(wm_manager_t *wm, wm_window_t *win, uint32_t *dest) {
    if (!win->visible) return;
    
    int screen_w = wm->width;
    int screen_h = wm->height;
    
    // Simple clipping
    int start_x = (win->x < 0) ? 0 : win->x;
    int start_y = (win->y < 0) ? 0 : win->y;
    int end_x = (win->x + win->width > screen_w) ? screen_w : win->x + win->width;
    int end_y = (win->y + win->height > screen_h) ? screen_h : win->y + win->height;
    
    // Decorations (Title bar) height = 20
    int dec_h = 20;
    
    // Draw decorations (very basic)
    int dec_start_y = start_y;
    if (win->y >= 0 && win->y < screen_h) { // if top is visible
        int title_color = win->focused ? 0xFF5E81AC : COLOR_TITLE_BG;
        for (int y = win->y; y < win->y + dec_h; y++) {
            if (y >= screen_h) break;
            for (int x = start_x; x < end_x; x++) {
                dest[y * screen_w + x] = title_color;
            }
        }
    }
    
    // Draw content
    for (int y = start_y; y < end_y; y++) {
        int win_y = y - (win->y + dec_h);
        if (win_y < 0 || win_y >= win->height) continue;
        
        for (int x = start_x; x < end_x; x++) {
            int win_x = x - win->x;
            uint32_t color = win->pixels[win_y * win->width + win_x];
            dest[y * screen_w + x] = color;
        }
    }
}

void wm_render(wm_manager_t *wm, uint32_t *output_buffer) {
    // 1. Clear background
    int size = wm->width * wm->height;
    for (int i = 0; i < size; i++) {
        output_buffer[i] = wm->background_color;
    }
    
    // 2. Draw windows (Painter's Algorithm: Bottom -> Top)
    for (int i = 0; i < wm->count; i++) {
        blit_window(wm, wm->windows[i], output_buffer);
    }
}
