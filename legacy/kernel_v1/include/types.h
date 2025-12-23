#ifndef TYPES_H
#define TYPES_H

// Define exact-width types for freestanding environment
typedef unsigned char uint8_t;
typedef signed char int8_t;
typedef unsigned short uint16_t;
typedef signed short int16_t;
typedef unsigned int uint32_t;
typedef signed int int32_t;
typedef unsigned long long uint64_t;
typedef signed long long int64_t;
typedef struct
{
    int16_t x;
    int16_t y;
} point_t;

// Structure to represent a rectangle on the screen
typedef struct
{
    int16_t x;
    int16_t y;
    int16_t width;
    int16_t height;
} rect_t;

// The gui_renderer_t needs the rect_t and point_t types,
// so it should be defined here as well.
typedef struct
{
    void (*draw_rect)(rect_t bounds, uint32_t color);
    void (*draw_text)(const char *text, point_t pos, uint32_t color);
} gui_renderer_t;

#define NULL ((void *)0)

#endif // TYPES_H
