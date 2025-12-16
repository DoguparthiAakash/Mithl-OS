#include "video.h"

static uint16_t *vga_buffer = (uint16_t *)0xB8000;
static int cursor_x = 0, cursor_y = 0;

void video_clear()
{
    for (int i = 0; i < 80 * 25; i++)
    {
        vga_buffer[i] = (0x07 << 8) | ' ';
    }
    cursor_x = 0;
    cursor_y = 0;
}

void video_putc(char c)
{
    if (c == '\n')
    {
        cursor_x = 0;
        cursor_y++;
    }
    else
    {
        vga_buffer[cursor_y * 80 + cursor_x] = (0x07 << 8) | c;
        cursor_x++;
    }
    if (cursor_x >= 80)
    {
        cursor_x = 0;
        cursor_y++;
    }
}
