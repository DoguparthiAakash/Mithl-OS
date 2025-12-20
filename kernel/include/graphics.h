#ifndef GRAPHICS_H
#define GRAPHICS_H
#include "types.h"

/* Color definitions for macOS-like theme */
#ifndef COLOR_WHITE
#define COLOR_WHITE         0xFFFFFFFF
#endif
#ifndef COLOR_BLACK
#define COLOR_BLACK         0xFF000000
#endif
// ... keep other colors if they don't conflict or use ifndef ...

/* Linear framebuffer description */
typedef struct {
    uint8_t  *address;  /* Base address of framebuffer */
    uint32_t  width;    /* Pixels */
    uint32_t  height;   /* Pixels */
    uint32_t  pitch;    /* Bytes per scanline */
    uint32_t  bpp;      /* Bits per pixel: 24 or 32 typical */
} framebuffer_t;

/* Point, rect, gui_renderer are in types.h */

#define VGA_COLOR_BLUE 0x01
#define VGA_COLOR_GREEN 0x02
#define VGA_COLOR_CYAN 0x03
#define VGA_COLOR_RED 0x04
#define VGA_COLOR_MAGENTA 0x05
#define VGA_COLOR_BROWN 0x06
#define VGA_COLOR_LIGHT_GRAY 0x07
#define VGA_COLOR_DARK_GRAY 0x08
#define VGA_COLOR_LIGHT_BLUE 0x09
#define VGA_COLOR_LIGHT_GREEN 0x0A
#define VGA_COLOR_LIGHT_CYAN 0x0B
#define VGA_COLOR_LIGHT_RED 0x0C
#define VGA_COLOR_LIGHT_MAGENTA 0x0D
#define VGA_COLOR_LIGHT_BROWN 0x0E
#define VGA_COLOR_WHITE 0x0F

/* Global framebuffer instance (defined in graphics.c) */
extern framebuffer_t fb;

/* Init from bootloader (not used unless you pass real values) */
void graphics_init(uint32_t width, uint32_t height,
                   uint32_t pitch, uint32_t bpp, void *addr);

/* Pixel helpers */
void set_pixel(int x, int y, uint32_t argb);
uint32_t get_pixel(int x, int y);

/* Back-compat for earlier typo */
void set_pixe(int x, int y, uint32_t argb);

void clear_screen(uint32_t argb);
void graphics_swap_buffers(void);
void graphics_fade_in(void);
void graphics_copy_rect(int x, int y, int w, int h);
void graphics_set_clip(rect_t rect);


/* Drawing primitives */
void draw_pixel(int x, int y, uint32_t color);
void draw_line(int x1, int y1, int x2, int y2, uint32_t color);
void draw_rect(rect_t rect, uint32_t color);
void draw_round_rect(rect_t rect, int radius, uint32_t color);
void draw_rect_filled(rect_t rect, uint32_t color);
void draw_circle(int x, int y, int radius, uint32_t color);
void draw_circle_filled(int x, int y, int radius, uint32_t color);
void draw_circle_quadrant_filled(int x, int y, int radius, int quadrant, uint32_t color);
void draw_rounded_rect_filled(rect_t rect, int radius, uint32_t color);
void apply_glass_effect(rect_t rect, int radius);



/* Text rendering */
void draw_text(const char *text, int x, int y, uint32_t color, uint32_t size);
void draw_text_exp(const char *text, int x, int y, uint32_t color, uint32_t size, int spacing);
void draw_text_sf(const char *text, int x, int y, uint32_t color);
void draw_text_sf_mono(const char *text, int x, int y, uint32_t color);
int get_text_width_sf(const char *text);
void draw_text_centered(const char *text, rect_t rect, uint32_t color, uint32_t size);
void graphics_draw_image(int x, int y, int w, int h, const uint32_t* data);

void draw_boot_logo(void);

/* Utility functions */
uint32_t blend_colors(uint32_t color1, uint32_t color2, uint8_t alpha);
uint32_t darken_color(uint32_t color, uint8_t factor);
uint32_t lighten_color(uint32_t color, uint8_t factor);

void draw_cursor_icon(int x, int y);
void draw_icon(int x, int y, const uint32_t* icon_data);
void draw_boot_logo(void);

#define MAKE_VGA_COLOR(fg, bg) (((bg) << 4) | (fg))
#endif /* GRAPHICS_H */
