#include "graphics.h"
#include "memory.h"
#include "string.h"

/* Simple abs function for freestanding mode */
static int abs(int x) {
    return x < 0 ? -x : x;
}

framebuffer_t fb = { 0 };

// Backbuffer for double buffering
// Changed to dynamic pointer to handle variable resolutions
static uint8_t *backbuffer = NULL;

void graphics_init(uint32_t width, uint32_t height,
                   uint32_t pitch, uint32_t bpp, void *addr)
{
    fb.width   = width;
    fb.height  = height;
    fb.pitch   = pitch;
    fb.bpp     = bpp;
    fb.address = (uint8_t*)addr;
    
    // Allocate backbuffer
    // Use 4 bytes per pixel for safety (even if bpp is 24, aligned to 4 is better)
    uint32_t buffer_size = width * height * 4;
    backbuffer = (uint8_t*)memory_alloc(buffer_size);
    
    // Clear backbuffer initially
    if (backbuffer) {
         memset(backbuffer, 0, buffer_size); 
    }
    
    // Init clip
    graphics_set_clip((rect_t){0, 0, width, height});
}

// Helper to copy backbuffer to VRAM
#include "string.h"
// Helper to copy backbuffer to VRAM
#include "string.h"

// Copy a specific rectangle from backbuffer to VRAM
void graphics_copy_rect(int x, int y, int w, int h) {
    if (!fb.address || !backbuffer) return;
    
    // Bounds check
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x + w > (int)fb.width) w = (int)fb.width - x;
    if (y + h > (int)fb.height) h = (int)fb.height - y;
    if (w <= 0 || h <= 0) return;

    uint32_t bytes_per_pixel = fb.bpp / 8;
    uint32_t row_size = w * bytes_per_pixel;
    uint32_t fb_pitch = fb.pitch;
    uint32_t internal_pitch = fb.width * bytes_per_pixel;
    
    for (int i = 0; i < h; i++) {
        void* dst = fb.address + (y + i) * fb_pitch + x * bytes_per_pixel;
        void* src = backbuffer + (y + i) * internal_pitch + x * bytes_per_pixel;
        memcpy(dst, src, row_size);
    }
}

void graphics_swap_buffers(void)
{
    if (!fb.address) return;
    graphics_copy_rect(0, 0, fb.width, fb.height);
}

static inline void write_pixel_32(uint8_t *p, uint32_t src)// argb
{
    // Check alpha
    uint8_t alpha = (src >> 24) & 0xFF;
    if (alpha == 0) return; // Transparent
    if (alpha == 255) {
        *(uint32_t*)p = src;
        return;
    }
    
    // Blend with existing
    uint32_t dest = *(uint32_t*)p;
    
    uint8_t s_r = (src >> 16) & 0xFF;
    uint8_t s_g = (src >> 8) & 0xFF;
    uint8_t s_b = (src) & 0xFF;
    
    uint8_t d_r = (dest >> 16) & 0xFF;
    uint8_t d_g = (dest >> 8) & 0xFF;
    uint8_t d_b = (dest) & 0xFF;
    
    uint32_t inv_alpha = 256 - alpha; 
    
    uint8_t out_r = (s_r * alpha + d_r * inv_alpha) >> 8;
    uint8_t out_g = (s_g * alpha + d_g * inv_alpha) >> 8;
    uint8_t out_b = (s_b * alpha + d_b * inv_alpha) >> 8;
    
    *(uint32_t*)p = (0xFF << 24) | (out_r << 16) | (out_g << 8) | out_b;
}

static inline void write_pixel_24(uint8_t *p, uint32_t argb)
{
    /* Write B,G,R (adjust if your mode is RGB) */
    p[0] = (uint8_t)(argb      );
    p[1] = (uint8_t)(argb >>  8);
    p[2] = (uint8_t)(argb >> 16);
}

static inline void write_pixel_8(uint8_t *p, uint32_t argb)
{
    // For VGA mode 13h, we only use the lowest 8 bits as a color index
    *p = (uint8_t)(argb & 0xFF);
}

void set_pixel(int x, int y, uint32_t argb)
{
    if (!fb.address || !backbuffer) return;
    if (x < 0 || y < 0) return;
    if ((uint32_t)x >= fb.width || (uint32_t)y >= fb.height) return;

    uint32_t bytes_per_pixel = fb.bpp / 8;
    // Use backbuffer instead of fb.address
    // uint8_t *row   = fb.address + (uint32_t)y * fb.pitch;
    // We treat backbuffer as tightly packed (pitch = width * bpp/8)
    uint32_t internal_pitch = fb.width * bytes_per_pixel;
    uint8_t *row = backbuffer + (uint32_t)y * internal_pitch;
    uint8_t *pixel = row + (uint32_t)x * bytes_per_pixel;

    if (fb.bpp == 32) {
        write_pixel_32(pixel, argb);
    } else if (fb.bpp == 24) {
        write_pixel_24(pixel, argb);
    } else if (fb.bpp == 8) {
        write_pixel_8(pixel, argb);
    }
}

/* Back-compat for earlier typo */
void set_pixe(int x, int y, uint32_t argb)
{
    set_pixel(x, y, argb);
}

void clear_screen(uint32_t argb)
{
    if (!fb.address || !backbuffer) return;

    uint32_t bytes_per_pixel = fb.bpp / 8;
    uint32_t internal_pitch = fb.width * bytes_per_pixel;

    for (uint32_t y = 0; y < fb.height; ++y) {
        uint8_t *row = backbuffer + y * internal_pitch;
        for (uint32_t x = 0; x < fb.width; ++x) {
            uint8_t *p = row + x * bytes_per_pixel;
            if (fb.bpp == 32) {
                write_pixel_32(p, argb);
            } else if (fb.bpp == 24) {
                write_pixel_24(p, argb);
            } else if (fb.bpp == 8) {
                write_pixel_8(p, argb);
            }
        }
    }
}

/* Additional graphics functions for GUI */

static inline uint32_t read_pixel_32(uint8_t *p)
{
    return *(uint32_t*)p;
}

static inline uint32_t read_pixel_24(uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16);
}

static inline uint32_t read_pixel_8(uint8_t *p)
{
    return (uint32_t)*p;
}

uint32_t get_pixel(int x, int y)
{
    if (!fb.address || !backbuffer) return 0;
    if (x < 0 || y < 0) return 0;
    if ((uint32_t)x >= fb.width || (uint32_t)y >= fb.height) return 0;

    uint32_t bytes_per_pixel = fb.bpp / 8;
    // uint8_t *row   = fb.address + (uint32_t)y * fb.pitch;
    uint32_t internal_pitch = fb.width * bytes_per_pixel;
    uint8_t *row = backbuffer + (uint32_t)y * internal_pitch;
    uint8_t *pixel = row + (uint32_t)x * bytes_per_pixel;

    if (fb.bpp == 32) {
        return read_pixel_32(pixel);
    } else if (fb.bpp == 24) {
        return read_pixel_24(pixel);
    } else if (fb.bpp == 8) {
        return (uint32_t)read_pixel_8(pixel);
    }
    return 0;
}

/* Drawing primitives */
void draw_line(int x1, int y1, int x2, int y2, uint32_t color)
{
    int dx = x2 - x1;
    int dy = y2 - y1;
    int ux = ((dx > 0) << 1) - 1;
    int uy = ((dy > 0) << 1) - 1;
    int x = x1, y = y1, eps;

    eps = 0; dx = abs(dx); dy = abs(dy);
    if (dx > dy) {
        for (x = x1; x != x2; x += ux) {
            set_pixel(x, y, color);
            eps += dy;
            if ((eps << 1) >= dx) {
                y += uy; eps -= dx;
            }
        }
    } else {
        for (y = y1; y != y2; y += uy) {
            set_pixel(x, y, color);
            eps += dx;
            if ((eps << 1) >= dy) {
                x += ux; eps -= dy;
            }
        }
    }
}

void draw_rect_outline(int x, int y, int width, int height, uint32_t color) {
    draw_rect_filled((rect_t){x, y, width, 1}, color);            // Top
    draw_rect_filled((rect_t){x, y + height - 1, width, 1}, color); // Bottom
    draw_rect_filled((rect_t){x, y, 1, height}, color);             // Left
    draw_rect_filled((rect_t){x + width - 1, y, 1, height}, color); // Right
}

void draw_rect(rect_t rect, uint32_t color) {
    draw_rect_filled(rect, color);
}

// Simple rounded rect implementation
void draw_round_rect(rect_t rect, int radius, uint32_t color) {
    int x = rect.x, y = rect.y, w = rect.width, h = rect.height;
    // Top
    draw_line(x + radius, y, x + w - radius, y, color);
    // Bottom
    draw_line(x + radius, y + h - 1, x + w - radius, y + h - 1, color);
    // Left
    draw_line(x, y + radius, x, y + h - radius, color);
    // Right
    draw_line(x + w - 1, y + radius, x + w - 1, y + h - radius, color);
    
    // Corners (simple pixels for small radius or 45deg lines)
    // For radius 5, let's just draw diagonal or manual pixels. 
    // Simplified corner:
    if (radius > 0) {
        // TL
        set_pixel(x + 1, y + 1, color);
        // TR
        set_pixel(x + w - 2, y + 1, color);
        // BL
        set_pixel(x + 1, y + h - 2, color);
        // BR
        set_pixel(x + w - 2, y + h - 2, color);
    }
}

static rect_t clip_rect = {0, 0, 0, 0};

void graphics_set_clip(rect_t rect) {
    // Clamp to screen
    if (rect.x < 0) rect.x = 0;
    if (rect.y < 0) rect.y = 0;
    if (rect.x + rect.width > (int)fb.width) rect.width = fb.width - rect.x;
    if (rect.y + rect.height > (int)fb.height) rect.height = fb.height - rect.y;
    clip_rect = rect;
}

static inline int rect_intersect(rect_t *r1, rect_t *r2, rect_t *out) {
    int x1 = (r1->x > r2->x) ? r1->x : r2->x;
    int y1 = (r1->y > r2->y) ? r1->y : r2->y;
    int x2_1 = r1->x + r1->width;
    int x2_2 = r2->x + r2->width;
    int x2 = (x2_1 < x2_2) ? x2_1 : x2_2;
    int y2_1 = r1->y + r1->height;
    int y2_2 = r2->y + r2->height;
    int y2 = (y2_1 < y2_2) ? y2_1 : y2_2;
    
    if (x1 < x2 && y1 < y2) {
        out->x = x1;
        out->y = y1;
        out->width = x2 - x1;
        out->height = y2 - y1;
        return 1;
    }
    return 0;
}

void draw_rect_filled(rect_t rect, uint32_t color)
{
    rect_t draw_rect;
    if (clip_rect.width == 0) {
        clip_rect.width = fb.width; clip_rect.height = fb.height;
    }
    
    if (!rect_intersect(&rect, &clip_rect, &draw_rect)) return;
    if (!backbuffer) return;

    // Check Alpha
    uint8_t alpha = (color >> 24) & 0xFF;
    if (alpha == 0) return; // Invisible

    // Optimization: If Opaque, allow fast 32bpp fill
    int is_opaque = (alpha == 255);

    for (int y = draw_rect.y; y < draw_rect.y + draw_rect.height; y++) {
         uint32_t bytes_per_pixel = fb.bpp / 8;
         uint32_t internal_pitch = fb.width * bytes_per_pixel;
         uint8_t *row = backbuffer + y * internal_pitch;
         uint8_t *start = row + draw_rect.x * bytes_per_pixel;
         
         for (int x = 0; x < draw_rect.width; x++) {
             if (fb.bpp == 32) {
                 if (is_opaque) {
                     *(uint32_t*)(start + x * 4) = color;
                 } else {
                     // Inline Blending for performance
                     uint32_t *p = (uint32_t*)(start + x * 4);
                     uint32_t bg = *p;
                     
                     // Optimization: If we assume premultiplied, etc. but we stick to standard
                     // out = alpha * fg + (1-alpha) * bg
                     
                     // Unpack BG
                     uint8_t r_bg = (bg >> 16) & 0xFF;
                     uint8_t g_bg = (bg >> 8) & 0xFF;
                     uint8_t b_bg = bg & 0xFF;
                     
                     // Unpack FG
                     uint8_t r_fg = (color >> 16) & 0xFF;
                     uint8_t g_fg = (color >> 8) & 0xFF;
                     uint8_t b_fg = color & 0xFF;
                     
                     // Blend (Fast approx 256 scale)
                     uint32_t inv_alpha = 256 - alpha;
                     uint8_t r_out = (r_fg * alpha + r_bg * inv_alpha) >> 8;
                     uint8_t g_out = (g_fg * alpha + g_bg * inv_alpha) >> 8;
                     uint8_t b_out = (b_fg * alpha + b_bg * inv_alpha) >> 8;
                     
                     *p = (0xFF << 24) | (r_out << 16) | (g_out << 8) | b_out;
                 }
             }
             else {
                 set_pixel(draw_rect.x + x, y, color); // Fallback handles blending
             }
         }
    }
}

void draw_circle(int x, int y, int radius, uint32_t color)
{
    int x1 = 0, y1 = radius;
    int d = 3 - 2 * radius;
    
    while (y1 >= x1) {
        set_pixel(x + x1, y + y1, color);
        set_pixel(x + x1, y - y1, color);
        set_pixel(x - x1, y + y1, color);
        set_pixel(x - x1, y - y1, color);
        set_pixel(x + y1, y + x1, color);
        set_pixel(x + y1, y - x1, color);
        set_pixel(x - y1, y + x1, color);
        set_pixel(x - y1, y - x1, color);
        
        if (d < 0) {
            d += 4 * x1 + 6;
        } else {
            d += 4 * (x1 - y1) + 10;
            y1--;
        }
        x1++;
    }
}

// Helper for rounded corners
// Helper to apply glass effect (lighten + blur approx) to a rect
void apply_glass_effect(rect_t rect, int radius) {
    // 1. Calculate Clip Intersection
    rect_t draw_rect;
    if (clip_rect.width == 0) {
        clip_rect.width = fb.width; clip_rect.height = fb.height;
    }
    
    if (!rect_intersect(&rect, &clip_rect, &draw_rect)) return;

    // 2. Iterate ONLY the intersected area
    uint32_t bytes_per_pixel = fb.bpp / 8;
    uint32_t internal_pitch = fb.width * bytes_per_pixel;
    
    for (int y = draw_rect.y; y < draw_rect.y + draw_rect.height; y++) {
        for (int x = draw_rect.x; x < draw_rect.x + draw_rect.width; x++) {
             // 3. Roundness Check (Must use ORIGINAL rect coordinates)
             // We check against 'rect' bounds, not 'draw_rect'.
             if (radius > 0) {
                 // TL
                 if (x < rect.x + radius && y < rect.y + radius) {
                     int dx = x - (rect.x + radius);
                     int dy = y - (rect.y + radius);
                     if (dx*dx + dy*dy > radius*radius) continue;
                 }
                 // TR
                 if (x >= rect.x + rect.width - radius && y < rect.y + radius) {
                     int dx = x - (rect.x + rect.width - radius - 1);
                     int dy = y - (rect.y + radius);
                     if (dx*dx + dy*dy > radius*radius) continue;
                 }
                 // BL
                 if (x < rect.x + radius && y >= rect.y + rect.height - radius) {
                     int dx = x - (rect.x + radius);
                     int dy = y - (rect.y + rect.height - radius - 1);
                     if (dx*dx + dy*dy > radius*radius) continue;
                 }
                 // BR
                 if (x >= rect.x + rect.width - radius && y >= rect.y + rect.height - radius) {
                     int dx = x - (rect.x + rect.width - radius - 1);
                     int dy = y - (rect.y + rect.height - radius - 1);
                     if (dx*dx + dy*dy > radius*radius) continue;
                 }
             }

             // Read Pixel
             uint8_t *p = backbuffer + y * internal_pitch + x * bytes_per_pixel;
             uint32_t pixel = 0;
             if (fb.bpp == 32) pixel = *(uint32_t*)p;
             else pixel = read_pixel_24(p);
             
             // Tint with White (0xFFFFFF) at alpha 70 (approx 27%)
             uint8_t r = (pixel >> 16) & 0xFF;
             uint8_t g = (pixel >> 8) & 0xFF;
             uint8_t b = pixel & 0xFF;
             
             int A = 70;
             r = (255 * A + r * (255 - A)) / 255;
             g = (255 * A + g * (255 - A)) / 255;
             b = (255 * A + b * (255 - A)) / 255;
             
             uint32_t final = 0xFF000000 | (r << 16) | (g << 8) | b;
             
             if (fb.bpp == 32) *(uint32_t*)p = final;
             else write_pixel_24(p, final);
        }
    }
}

// Helper for rounded corners
// Helper for intersection - defined earlier in file?
// Use rect_intersect helper
void graphics_draw_image(int x, int y, int w, int h, const uint32_t* data) {
    if (!data) return;
    if (!backbuffer) return;

    
    rect_t img_rect = {x, y, w, h};
    rect_t draw_rect;
    
    // Bounds check?
    if (clip_rect.width == 0) {
        clip_rect.width = fb.width; clip_rect.height = fb.height;
    }
    
    if (!rect_intersect(&img_rect, &clip_rect, &draw_rect)) return;
    
    uint32_t bytes_per_pixel = fb.bpp / 8;
    uint32_t internal_pitch = fb.width * bytes_per_pixel;
    
    for (int dy = 0; dy < draw_rect.height; dy++) {
        int screen_y = draw_rect.y + dy;
        int src_y = screen_y - y; // offset into source image
        
        uint32_t *row_ptr = (uint32_t*)(backbuffer + screen_y * internal_pitch);
        
        for (int dx = 0; dx < draw_rect.width; dx++) {
             int screen_x = draw_rect.x + dx;
             int src_x = screen_x - x; // offset into source image
             
             // Source index
             uint32_t src_pixel = data[src_y * w + src_x];
             uint8_t alpha = (src_pixel >> 24) & 0xFF;

             if (alpha == 0) {
                 continue; // Fully transparent, skip
             }
             else if (alpha == 255) {
                 // Fully opaque, copy directly
                 if (fb.bpp == 32) {
                     row_ptr[screen_x] = src_pixel;
                 } else {
                     set_pixel(screen_x, screen_y, src_pixel);
                 }
             }
             else {
                 // Alpha blending
                 // SrcOver: Out = Src * A + Dst * (1 - A)
                 uint32_t dest_pixel;
                 if (fb.bpp == 32) {
                     dest_pixel = row_ptr[screen_x];
                 } else {
                     // Fallback, though we really only support 32bpp for this logic effectively
                     continue; 
                 }
                 
                 uint8_t s_r = (src_pixel >> 16) & 0xFF;
                 uint8_t s_g = (src_pixel >> 8) & 0xFF;
                 uint8_t s_b = (src_pixel) & 0xFF;
                 
                 uint8_t d_r = (dest_pixel >> 16) & 0xFF;
                 uint8_t d_g = (dest_pixel >> 8) & 0xFF;
                 uint8_t d_b = (dest_pixel) & 0xFF;
                 
                 // Fast blend approx: (s * a + d * (255 - a)) / 255
                 // Using >> 8 is faster but slightly inaccurate (255->256), 
                 // (x * a + y * (256 - a)) >> 8 is standard fast approx
                 
                 uint32_t inv_alpha = 256 - alpha; // Approximation for speed
                 // actually standard is 255, but >>8 requires 256 scale.
                 // Correct logic for >> 8: (src * alpha + dst * (256-alpha)) >> 8
                 
                 uint8_t out_r = (s_r * alpha + d_r * inv_alpha) >> 8;
                 uint8_t out_g = (s_g * alpha + d_g * inv_alpha) >> 8;
                 uint8_t out_b = (s_b * alpha + d_b * inv_alpha) >> 8;
                 
                 uint32_t out_pixel = (0xFF << 24) | (out_r << 16) | (out_g << 8) | out_b;
                 row_ptr[screen_x] = out_pixel;
             }
        }
    }
}    

// Helper to separate alpha blending logic
static void blend_pixel(int x, int y, uint32_t color) {
    if (x < 0 || x >= (int)fb.width || y < 0 || y >= (int)fb.height) return;
    
    // Check Alpha
    uint8_t alpha = (color >> 24) & 0xFF;
    if (alpha == 0) return; 

    uint32_t bytes_per_pixel = fb.bpp / 8;
    uint8_t *p = backbuffer + (y * fb.width + x) * bytes_per_pixel;

    if (alpha == 255) {
         if (fb.bpp == 32) *(uint32_t*)p = color;
         else set_pixel(x, y, color);
    } else {
         if (fb.bpp == 32) {
             uint32_t bg = *(uint32_t*)p;
             
             uint8_t r_bg = (bg >> 16) & 0xFF;
             uint8_t g_bg = (bg >> 8) & 0xFF;
             uint8_t b_bg = bg & 0xFF;
             
             uint8_t r_fg = (color >> 16) & 0xFF;
             uint8_t g_fg = (color >> 8) & 0xFF;
             uint8_t b_fg = color & 0xFF;
             
             uint32_t inv_alpha = 256 - alpha;
             uint8_t r_out = (r_fg * alpha + r_bg * inv_alpha) >> 8;
             uint8_t g_out = (g_fg * alpha + g_bg * inv_alpha) >> 8;
             uint8_t b_out = (b_fg * alpha + b_bg * inv_alpha) >> 8;
             
             *(uint32_t*)p = (0xFF << 24) | (r_out << 16) | (g_out << 8) | b_out;
         } else {
             set_pixel(x, y, color); 
         }
    }
}

void draw_circle_quadrant_filled(int cx, int cy, int r, int quadrant, uint32_t color) {
    for (int dy = 0; dy <= r; dy++) {
        for (int dx = 0; dx <= r; dx++) {
            if (dx*dx + dy*dy <= r*r) {
                int x = 0, y = 0;
                // Quadrants: 0=TR, 1=TL, 2=BL, 3=BR
                if (quadrant == 0) { x = cx + dx; y = cy - dy; }
                else if (quadrant == 1) { x = cx - dx; y = cy - dy; }
                else if (quadrant == 2) { x = cx - dx; y = cy + dy; }
                else if (quadrant == 3) { x = cx + dx; y = cy + dy; }
                
                blend_pixel(x, y, color);
            }
        }
    }
}

void draw_rounded_rect_filled(rect_t rect, int radius, uint32_t color)
{
    int x = rect.x;
    int y = rect.y;
    int w = rect.width;
    int h = rect.height;
    
    // Clamp radius
    if (radius > w/2) radius = w/2;
    if (radius > h/2) radius = h/2;

    // Draw central crosses
    rect_t r1 = {x + radius, y, w - 2*radius, h};
    draw_rect_filled(r1, color);
    
    rect_t r2 = {x, y + radius, radius, h - 2*radius};
    draw_rect_filled(r2, color);
    
    rect_t r3 = {x + w - radius, y + radius, radius, h - 2*radius};
    draw_rect_filled(r3, color);
    
    // Draw corners
    // TL
    draw_circle_quadrant_filled(x + radius, y + radius, radius, 1, color);
    // TR
    draw_circle_quadrant_filled(x + w - radius - 1, y + radius, radius, 0, color);
    // BL
    draw_circle_quadrant_filled(x + radius, y + h - radius - 1, radius, 2, color);
    // BR
    draw_circle_quadrant_filled(x + w - radius - 1, y + h - radius - 1, radius, 3, color);
}

void draw_circle_filled(int x, int y, int radius, uint32_t color)
{
    for (int dy = -radius; dy <= radius; dy++) {
        for (int dx = -radius; dx <= radius; dx++) {
            if (dx * dx + dy * dy <= radius * radius) {
                set_pixel(x + dx, y + dy, color);
            }
        }
    }
}

/* Simple bitmap font for text rendering */
#include "font8x8.h"
#include "font_sf.h"

// Use font_8x8_basic from header


void draw_text_exp(const char *text, int x, int y, uint32_t color, uint32_t size, int spacing)
{
    if (!text) return;
    
    // spacing is the avance per char. 
    // Usually char_width = 8 * size/8. If size=12, scale=1, width=8.
    // If we want tighter, we use spacing < 8.
    
    int scale = size / 8;
    if (scale < 1) scale = 1;

    for (int i = 0; text[i]; i++) {
        unsigned char c = (unsigned char)text[i];
        if (c >= 128) c = '?';
        
        // Draw character bitmap
        for (int cy = 0; cy < 8; cy++) {
            for (int cx = 0; cx < 8; cx++) {
                if (font_8x8_basic[c][cy] & (1 << (7 - cx))) {
                    for (int sy = 0; sy < scale; sy++) {
                        for (int sx = 0; sx < scale; sx++) {
                            // Draw relative to current char pos
                            set_pixel(x + (i * spacing * scale) + cx * scale + sx,
                                    y + cy * scale + sy, color);
                        }
                    }
                }
            }
        }
    }
}



// Variable width text drawing for San Francisco
void draw_text_sf(const char *text, int x, int y, uint32_t color)
{
    if (!text) return;
    
    int cursor_x = x;
    
    for (int i = 0; text[i]; i++) {
        unsigned char c = (unsigned char)text[i];
        if (c > 127) c = '?';
        
        // Get width and data
        int w = font_sf_widths[c];
        const uint16_t *bitmap = font_sf_data[c];
        
        // Draw Bitmap (16 rows)
        for (int r = 0; r < SF_HEIGHT; r++) {
            uint16_t row_data = bitmap[r];
            // MSB is left-most pixel. 
            
            for (int col = 0; col < 16; col++) {
               if (col >= w) break; 
               
               if (row_data & (1 << (15 - col))) {
                   set_pixel(cursor_x + col, y + r, color);
               }
            }
        }
        // Advance cursor with 1px padding
        cursor_x += w + 1; 
    }
}

// Monospace wrapper for SF font (centers glyphs in fixed width cell)
void draw_text_sf_mono(const char *text, int x, int y, uint32_t color)
{
    if (!text) return;
    
    int cell_w = 11; // Fixed cell width
    int cursor_x = x;
    
    for (int i = 0; text[i]; i++) {
        unsigned char c = (unsigned char)text[i];
        if (c > 127) c = '?';
        
        int w = font_sf_widths[c];
        const uint16_t *bitmap = font_sf_data[c];
        
        // Center in cell
        int offset_x = (cell_w - w) / 2;
        if (offset_x < 0) offset_x = 0; // Overlap left/right if too wide
        
        // Draw Bitmap
        for (int r = 0; r < SF_HEIGHT; r++) {
            uint16_t row_data = bitmap[r];
            for (int col = 0; col < 16; col++) {
               if (col >= w) break;
               
               if (row_data & (1 << (15 - col))) {
                   // Clip to cell width if strict, or allow spill
                   // Allowing spill is better for 'W' reading, but might overlap next char.
                   // Since Z-order isn't per char, overlap is just addition. Be careful.
                   set_pixel(cursor_x + offset_x + col, y + r, color);
               }
            }
        }
        cursor_x += cell_w;
    }
}

// Replaces existing draw_text.
void draw_text(const char *text, int x, int y, uint32_t color, uint32_t size)
{
    // Use modern font by default as requested ("clean font for OS as default")
    (void)size; // Unused
    draw_text_sf(text, x, y, color);
}

// Legacy 8x8 for specific needs if any
void draw_text_8x8(const char *text, int x, int y, uint32_t color, uint32_t size) {
     draw_text_exp(text, x, y, color, size, 8);
}


int get_text_width_sf(const char *text) {
    if (!text) return 0;
    int w_total = 0;
    for (int i = 0; text[i]; i++) {
        unsigned char c = (unsigned char)text[i];
        if (c > 127) c = '?';
        w_total += font_sf_widths[c] + 1; // +1 for padding
    }
    return w_total;
}

void draw_text_centered(const char *text, rect_t rect, uint32_t color, uint32_t size)
{
    if (!text) return;
    
    // Calculate text width (approximate)
    int text_width = 0;
    for (int i = 0; text[i]; i++) {
        text_width += 8 * size / 8;
    }
    
    int x = rect.x + (rect.width - text_width) / 2;
    int y = rect.y + (rect.height - 8 * size / 8) / 2;
    
    draw_text(text, x, y, color, size);
}

/* Utility functions */
uint32_t blend_colors(uint32_t color1, uint32_t color2, uint8_t alpha)
{
    uint8_t r1 = (color1 >> 16) & 0xFF;
    uint8_t g1 = (color1 >> 8) & 0xFF;
    uint8_t b1 = color1 & 0xFF;
    
    uint8_t r2 = (color2 >> 16) & 0xFF;
    uint8_t g2 = (color2 >> 8) & 0xFF;
    uint8_t b2 = color2 & 0xFF;
    
    uint8_t r = (r1 * (255 - alpha) + r2 * alpha) / 255;
    uint8_t g = (g1 * (255 - alpha) + g2 * alpha) / 255;
    uint8_t b = (b1 * (255 - alpha) + b2 * alpha) / 255;
    
    return 0xFF000000 | (r << 16) | (g << 8) | b;
}

uint32_t darken_color(uint32_t color, uint8_t factor)
{
    uint8_t r = ((color >> 16) & 0xFF) * factor / 255;
    uint8_t g = ((color >> 8) & 0xFF) * factor / 255;
    uint8_t b = (color & 0xFF) * factor / 255;
    
    return 0xFF000000 | (r << 16) | (g << 8) | b;
}

uint32_t lighten_color(uint32_t color, uint8_t factor)
{
    uint8_t r = ((color >> 16) & 0xFF) + (255 - ((color >> 16) & 0xFF)) * factor / 255;
    uint8_t g = ((color >> 8) & 0xFF) + (255 - ((color >> 8) & 0xFF)) * factor / 255;
    uint8_t b = (color & 0xFF) + (255 - (color & 0xFF)) * factor / 255;
    
    return 0xFF000000 | (r << 16) | (g << 8) | b;
}

#include "cursor_icon.h"

void draw_cursor_icon(int x, int y)
{
    int hot_x = CURSOR_HOT_X;
    int hot_y = CURSOR_HOT_Y;
    
    // Adjust x, y for hot spot
    int start_x = x - hot_x;
    int start_y = y - hot_y;
    
    for (int cy = 0; cy < CURSOR_HEIGHT; cy++) {
        for (int cx = 0; cx < CURSOR_WIDTH; cx++) {
            // Screen coordinates
            int sx = start_x + cx;
            int sy = start_y + cy;
            
            // Bounds check
            if (sx < 0 || sx >= (int)fb.width || sy < 0 || sy >= (int)fb.height) continue;
            
            // Get pixel from icon
            uint32_t icon_pixel = cursor_icon[cy * CURSOR_WIDTH + cx];
            uint8_t alpha = (icon_pixel >> 24) & 0xFF; // ARGB
            
            if (alpha == 0) continue; // Fully transparent
            
            if (alpha == 255) {
                // Opaque
                set_pixel(sx, sy, icon_pixel);
            } else {
                // Blend with background
                uint32_t bg_pixel = get_pixel(sx, sy);
                
                // Blend channels
                uint8_t r_bg = (bg_pixel >> 16) & 0xFF;
                uint8_t g_bg = (bg_pixel >> 8) & 0xFF;
                uint8_t b_bg = bg_pixel & 0xFF;
                
                uint8_t r_fg = (icon_pixel >> 16) & 0xFF;
                uint8_t g_fg = (icon_pixel >> 8) & 0xFF;
                uint8_t b_fg = icon_pixel & 0xFF;
                
                // Standard alpha blending: out = alpha * fg + (1-alpha) * bg
                // Alpha is 0-255
                uint8_t r_out = (r_fg * alpha + r_bg * (255 - alpha)) / 255;
                uint8_t g_out = (g_fg * alpha + g_bg * (255 - alpha)) / 255;
                uint8_t b_out = (b_fg * alpha + b_bg * (255 - alpha)) / 255;
                
                uint32_t blended = 0xFF000000 | (r_out << 16) | (g_out << 8) | b_out;
                set_pixel(sx, sy, blended);
            }
        }
    }
}

// Draw general icon (assuming 48x48 from icons_data.h defines)
#include "icons_data.h"

void draw_icon(int x, int y, const uint32_t* icon_data)
{
    if (!icon_data) return;
    graphics_draw_image(x, y, 48, 48, icon_data);
}

#include "bootlogo.h"

// Note: graphics_draw_image is defined earlier in the file

// Draw boot logo
void draw_boot_logo(void) {
    // 1. Clear Screen to Black
    clear_screen(0xFF000000); // Black opaque
    
    // 2. Draw Centered Logo
    // Uses dimensions from new bootlogo.h
    int logo_x = (fb.width - bootlogo_width) / 2;
    int logo_y = (fb.height - bootlogo_height) / 2;
    
    graphics_draw_image(logo_x, logo_y, bootlogo_width, bootlogo_height, bootlogo_data);
    
    // 3. Draw Loading Bar Overlay (macOS style: thin white/grey bar below logo)
    int bar_width = 200; 
    int bar_height = 6;
    int bar_x = (fb.width - bar_width) / 2;
    int bar_y = logo_y + bootlogo_height + 40; // Spacing below logo
    
    // Bar Background (Dark Grey)
    rect_t bar_bg = { bar_x, bar_y, bar_width, bar_height };
    draw_rounded_rect_filled(bar_bg, bar_height/2, 0xFF333333); 
    
    // Bar Fill Animation (Simulated)
    // In a real OS this would be linked to boot progress.
    graphics_swap_buffers();
    
    rect_t bar_fill = { bar_x, bar_y, 0, bar_height };
    for (int i = 0; i <= 100; i++) {
        // Clear bar area first to prevent overdraw artifacts if alpha used (though here it's opaque on opaque)
        // draw_rounded_rect_filled(bar_bg, bar_height/2, 0xFF333333); 
        
        bar_fill.width = (bar_width * i) / 100;
        draw_rounded_rect_filled(bar_fill, bar_height/2, 0xFFCCCCCC); // Light grey/white fill
        
        graphics_swap_buffers();
        
        // Short delay for visibility
        for(volatile int k=0; k<2000000; k++); 
    }
}

// Fade in from black to the current backbuffer content
void graphics_fade_in(void) {
    if (!fb.address || !backbuffer) return;

    // 1. Allocate a temp buffer to hold the CLEAN desktop image
    // We need this because we'll be modifying the backbuffer with a black overlay each frame,
    // and we need to restore the clean image for the next frame's lighter overlay.
    // Use 4 bytes per pixel for simplicity (alignment)
    uint32_t buffer_size = fb.width * fb.height * 4;
    uint8_t *clean_buffer = (uint8_t*)memory_alloc(buffer_size);
    
    if (!clean_buffer) return; // Allocation failed, skip fade
    
    // 2. Save current backbuffer (which has the Desktop drawn on it)
    // Adjust pitch if needed, but we assume compact backbuffer in memory_alloc
    // uint32_t bytes_per_pixel = fb.bpp / 8;
    // uint32_t compact_pitch = fb.width * bytes_per_pixel;
    
    // We can't just memcpy if pitch differs, but our backbuffer IS compact (see graphics_init)
    // "backbuffer = (uint8_t*)memory_alloc(buffer_size);" -> It's compact.
    memcpy(clean_buffer, backbuffer, buffer_size);
    
    // 3. Animation Loop
    // Alpha: 255 (Solid Black) -> 0 (Transparent/Done)
    // Step size determines speed.
    int step = 5; 
    
    rect_t screen_rect = {0, 0, fb.width, fb.height};
    
    for (int alpha = 255; alpha >= 0; alpha -= step) {
        // A. Restore clean desktop to backbuffer
        memcpy(backbuffer, clean_buffer, buffer_size);
        
        // B. Draw black overlay with current alpha
        // We use a helper or manual fill for speed
        if (alpha > 0) {
            uint32_t black_overlay = (alpha << 24) | 0x000000;
            draw_rect_filled(screen_rect, black_overlay);
        }
        
        // C. Present to Screen
        graphics_swap_buffers();
        
        // D. Delay
        // Adjust this loop for timing
        for(volatile int k=0; k<100000; k++);
    }
    
    // 4. Cleanup
    // restore purely clean one last time just in case
    memcpy(backbuffer, clean_buffer, buffer_size);
    graphics_swap_buffers();
    
    memory_free(clean_buffer);
}


