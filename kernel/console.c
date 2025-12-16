#include "console.h"
#include <stddef.h>

#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_MEMORY ((uint16_t *)0xB8000)

static uint16_t *vga_buffer = VGA_MEMORY;
static uint8_t cursor_x = 0;
static uint8_t cursor_y = 0;
static uint8_t console_color = 0x07; // light gray on black

static inline uint16_t vga_entry(char c, uint8_t color)
{
    return (uint16_t)c | ((uint16_t)color << 8);
}

void console_clear()
{
    for (size_t y = 0; y < VGA_HEIGHT; y++)
    {
        for (size_t x = 0; x < VGA_WIDTH; x++)
        {
            vga_buffer[y * VGA_WIDTH + x] = vga_entry(' ', console_color);
        }
    }
    cursor_x = cursor_y = 0;
}

void console_set_color(uint8_t fg, uint8_t bg)
{
    console_color = fg | (bg << 4);
}

void console_putc(char c)
{
    if (c == '\n')
    {
        cursor_x = 0;
        cursor_y++;
    }
    else
    {
        vga_buffer[cursor_y * VGA_WIDTH + cursor_x] = vga_entry(c, console_color);
        cursor_x++;
        if (cursor_x >= VGA_WIDTH)
        {
            cursor_x = 0;
            cursor_y++;
        }
    }

    if (cursor_y >= VGA_HEIGHT)
    {
        console_clear();
    }
}

void console_write(const char *str)
{
    while (*str)
    {
        console_putc(*str++);
    }
}

void console_write_len(const char *str, uint32_t len)
{
    for (uint32_t i = 0; i < len; i++)
    {
        console_putc(str[i]);
    }
}
