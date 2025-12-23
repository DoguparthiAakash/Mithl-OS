#pragma once

#include "types.h"

void video_clear();
void video_putc(char c);
void video_set_cursor(int x, int y);
void video_set_color(uint8_t fg, uint8_t bg);