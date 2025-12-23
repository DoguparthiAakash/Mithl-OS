#ifndef DESKTOP_H
#define DESKTOP_H

#include "types.h" 

void desktop_init(void);
void desktop_update(void);
void desktop_draw(void);

// Helper functions for drawing specific elements
void draw_taskbar(void);
void draw_start_menu(void);
void draw_icons(void);

void desktop_draw_rect(int x, int y, int w, int h);
void desktop_check_clock(void);
#endif
