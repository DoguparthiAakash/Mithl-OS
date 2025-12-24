#ifndef IMAGE_LOADER_H
#define IMAGE_LOADER_H

#include <stdint.h>

// Simple Image Structure (ARGB)
typedef struct {
    int width;
    int height;
    int channels; // usually 4 (ARGB)
    uint32_t *data; // Pixel data
} sys_image_t;

// Load a BMP file
// Returns NULL on failure
sys_image_t* load_bmp(const char *filename);

// Free image
void free_image(sys_image_t *img);

#endif
