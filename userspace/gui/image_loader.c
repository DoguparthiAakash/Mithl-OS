#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "image_loader.h"

#pragma pack(push, 1)
typedef struct {
    uint16_t type;              // Magic identifier: 0x4d42
    uint32_t size;              // File size in bytes
    uint16_t reserved1;         // Not used
    uint16_t reserved2;         // Not used
    uint32_t offset;            // Offset to image data in bytes
    uint32_t dib_header_size;   // DIB Header size in bytes
    int32_t  width_px;          // Width of the image
    int32_t  height_px;         // Height of image
    uint16_t num_planes;        // Number of color planes
    uint16_t bits_per_pixel;    // Bits per pixel
    uint32_t compression;       // Compression type
    uint32_t image_size_bytes;  // Image size in bytes
    int32_t  x_resolution_ppm;  // Pixels per meter
    int32_t  y_resolution_ppm;  // Pixels per meter
    uint32_t num_colors;        // Number of colors
    uint32_t important_colors;  // Important colors
} BMPHeader;
#pragma pack(pop)

sys_image_t* load_bmp(const char *filename) {
    FILE *f = fopen(filename, "rb");
    if (!f) {
        printf("[ImageLoader] Error: Could not open file %s\n", filename);
        return NULL;
    }

    BMPHeader header;
    if (fread(&header, sizeof(BMPHeader), 1, f) != 1) {
        printf("[ImageLoader] Error: Invalid BMP header\n");
        fclose(f);
        return NULL;
    }

    if (header.type != 0x4D42) { // 'BM'
        printf("[ImageLoader] Error: Not a BMP file\n");
        fclose(f);
        return NULL;
    }

    if (header.bits_per_pixel != 32 && header.bits_per_pixel != 24) {
        printf("[ImageLoader] Error: Only 24/32-bit BMPs supported (Found: %d)\n", header.bits_per_pixel);
        fclose(f);
        return NULL;
    }

    sys_image_t *img = calloc(1, sizeof(sys_image_t));
    img->width = header.width_px;
    img->height = abs(header.height_px); // Height can be negative (top-down)
    img->channels = 4;
    img->data = calloc(img->width * img->height, sizeof(uint32_t));

    fseek(f, header.offset, SEEK_SET);

    int row_padded = (header.width_px * (header.bits_per_pixel / 8) + 3) & (~3);
    unsigned char *row_data = malloc(row_padded);
    
    // Determine if top-down or bottom-up
    int start_y, end_y, step_y;
    if (header.height_px > 0) {
        // Bottom-up
        start_y = img->height - 1;
        end_y = -1;
        step_y = -1;
    } else {
        // Top-down
        start_y = 0;
        end_y = img->height;
        step_y = 1;
    }

    for (int y = start_y; y != end_y; y += step_y) {
        fread(row_data, row_padded, 1, f);
        for (int x = 0; x < img->width; x++) {
            int pixel_idx = x * (header.bits_per_pixel / 8);
            uint8_t b = row_data[pixel_idx];
            uint8_t g = row_data[pixel_idx + 1];
            uint8_t r = row_data[pixel_idx + 2];
            uint8_t a = (header.bits_per_pixel == 32) ? row_data[pixel_idx + 3] : 255;
            
            // Format: ARGB
            img->data[y * img->width + x] = (a << 24) | (r << 16) | (g << 8) | b;
        }
    }

    free(row_data);
    fclose(f);
    return img;
}

void free_image(sys_image_t *img) {
    if (img) {
        if (img->data) free(img->data);
        free(img);
    }
}
