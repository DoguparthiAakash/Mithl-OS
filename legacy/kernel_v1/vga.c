#include "vga.h"
#include "memory.h" // For memory_alloc
#include "string.h" // For memcpy

// VGA text mode buffer address (front buffer)
volatile uint16_t *vga_buffer_front = (volatile uint16_t *)VGA_ADDRESS;

// Back buffer in regular memory
uint16_t *vga_buffer_back = NULL;

// Initialize the back buffer
void vga_init_buffers(void)
{
    // Allocate memory for the back buffer
    vga_buffer_back = (uint16_t *)memory_alloc(VGA_BUFFER_SIZE);
    if (!vga_buffer_back)
    {
        // Handle memory allocation failure
        return;
    }
    // Clear the back buffer initially
    vga_clear_buffer();
}
void vga_draw_rect_buffer(rect_t bounds, uint32_t color)
{
    uint8_t attr = (uint8_t)color; // Correct conversion for text mode attribute
    for (int16_t y = bounds.y; y < bounds.y + bounds.height; y++)
    {
        for (int16_t x = bounds.x; x < bounds.x + bounds.width; x++)
        {
            vga_set_char_at_buffer(x, y, ' ', attr);
        }
    }
}

void vga_draw_text_buffer(const char *text, point_t pos, uint32_t color)
{
    uint8_t attr = (uint8_t)color; // Correct conversion for text mode attribute
    int16_t x = pos.x;
    for (size_t i = 0; text[i] != '\0'; i++)
    {
        vga_set_char_at_buffer(x + i, pos.y, text[i], attr);
    }
}
// Set a character in the back buffer
void vga_set_char_at_buffer(int16_t x, int16_t y, char c, uint8_t attr)
{
    if (x < 0 || x >= VGA_WIDTH || y < 0 || y >= VGA_HEIGHT || !vga_buffer_back)
        return;

    uint16_t entry = (attr << 8) | c;
    vga_buffer_back[y * VGA_WIDTH + x] = entry;
}

// Get char from the back buffer
char vga_get_char_buffer(int16_t x, int16_t y)
{
    if (x < 0 || x >= VGA_WIDTH || y < 0 || y >= VGA_HEIGHT || !vga_buffer_back)
        return 0;

    return (char)(vga_buffer_back[y * VGA_WIDTH + x] & 0x00FF);
}

// Get attribute from the back buffer
uint8_t vga_get_attr_buffer(int16_t x, int16_t y)
{
    if (x < 0 || x >= VGA_WIDTH || y < 0 || y >= VGA_HEIGHT || !vga_buffer_back)
        return 0;

    return (uint8_t)((vga_buffer_back[y * VGA_WIDTH + x] & 0xFF00) >> 8);
}

// Copy the back buffer to the front buffer (the screen)
void vga_sync_buffer(void)
{
    if (vga_buffer_back)
    {
        memcpy((void *)vga_buffer_front, (const void *)vga_buffer_back, VGA_BUFFER_SIZE);
    }
}

// Clear the back buffer
void vga_clear_buffer(void)
{
    if (vga_buffer_back)
    {
        // Assume default black background (0x00) and spaces (' ')
        uint16_t empty_char = (0x00 << 8) | ' ';
        for (size_t i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++)
        {
            vga_buffer_back[i] = empty_char;
        }
    }
}
