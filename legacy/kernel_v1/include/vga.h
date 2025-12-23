#ifndef VGA_H
#define VGA_H

#include "types.h"
#include "graphics.h" // For rect_t and point_t

#define VGA_ADDRESS 0xB8000
#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_BUFFER_SIZE (VGA_WIDTH * VGA_HEIGHT * 2)

// Functions to write to the back buffer
void vga_init_buffers(void);
void vga_set_char_at_buffer(int16_t x, int16_t y, char c, uint8_t attr);
char vga_get_char_buffer(int16_t x, int16_t y);
uint8_t vga_get_attr_buffer(int16_t x, int16_t y);
void vga_sync_buffer(void);
void vga_clear_buffer(void);

// Renderer functions for the back buffer
void vga_draw_rect_buffer(rect_t bounds, uint32_t color);
void vga_draw_text_buffer(const char *text, point_t pos, uint32_t color);

#endif // VGA_H