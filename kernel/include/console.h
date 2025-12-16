#ifndef CONSOLE_H
#define CONSOLE_H
#include "types.h"
#include <stddef.h>

// Initialize console (set VGA buffer, cursor, etc.)
void console_init();
// Print a single character
void console_putc(char c);
// Print a null-terminated string
void console_write(const char *str);
// Print a string with length
void console_write_len(const char *str, uint32_t len);
// Set text color
void console_set_color(uint8_t fg, uint8_t bg);
// Clear screen
void console_clear();

#endif // CONSOLE_H