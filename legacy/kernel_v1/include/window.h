#ifndef WINDOW_H
#define WINDOW_H

// GUI function prototypes
void gui_init(int screen_width, int screen_height, gui_renderer_t *renderer);
void gui_draw(void);
void gui_present(void);

#endif // WINDOW_H