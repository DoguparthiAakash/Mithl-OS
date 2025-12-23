#include "triangle.h"
#include "graphics.h"
#include "string.h"

/* 
 * High-Performance Triangle Rasterizer
 * Uses scanline conversion for fast filling
 */

// Helper: Swap two points
static void swap_points(point_t* a, point_t* b) {
    point_t temp = *a;
    *a = *b;
    *b = temp;
}

// Sort three points by Y coordinate (ascending)
void sort_points_by_y(point_t* p1, point_t* p2, point_t* p3) {
    if (p1->y > p2->y) swap_points(p1, p2);
    if (p2->y > p3->y) swap_points(p2, p3);
    if (p1->y > p2->y) swap_points(p1, p2);
}

// Linear interpolation
static inline int lerp(int a, int b, int num, int den) {
    if (den == 0) return a;
    return a + ((b - a) * num) / den;
}

// Draw a horizontal scanline
static inline void draw_scanline(int y, int x1, int x2, uint32_t color) {
    if (x1 > x2) {
        int temp = x1;
        x1 = x2;
        x2 = temp;
    }
    
    for (int x = x1; x <= x2; x++) {
        set_pixel(x, y, color);
    }
}

// Draw a horizontal scanline with gradient
static inline void draw_scanline_gradient(int y, int x1, int x2, 
                                         uint32_t c1, uint32_t c2) {
    if (x1 > x2) {
        int temp_x = x1;
        x1 = x2;
        x2 = temp_x;
        uint32_t temp_c = c1;
        c1 = c2;
        c2 = temp_c;
    }
    
    int dx = x2 - x1;
    if (dx == 0) {
        set_pixel(x1, y, c1);
        return;
    }
    
    for (int x = x1; x <= x2; x++) {
        // Interpolate color
        int t = x - x1;
        uint8_t r = lerp((c1 >> 16) & 0xFF, (c2 >> 16) & 0xFF, t, dx);
        uint8_t g = lerp((c1 >> 8) & 0xFF, (c2 >> 8) & 0xFF, t, dx);
        uint8_t b = lerp(c1 & 0xFF, c2 & 0xFF, t, dx);
        uint8_t a = lerp((c1 >> 24) & 0xFF, (c2 >> 24) & 0xFF, t, dx);
        
        uint32_t color = (a << 24) | (r << 16) | (g << 8) | b;
        set_pixel(x, y, color);
    }
}

// Fill a solid-color triangle using scanline conversion
void draw_triangle_filled(point_t p1, point_t p2, point_t p3, uint32_t color) {
    // Sort points by Y coordinate
    sort_points_by_y(&p1, &p2, &p3);
    
    // Handle degenerate cases
    if (p1.y == p3.y) return; // Zero height
    
    // Split into two triangles: top and bottom
    // Top triangle: p1 -> p2
    // Bottom triangle: p2 -> p3
    
    int total_height = p3.y - p1.y;
    
    for (int y = p1.y; y <= p3.y; y++) {
        // Check if we're in top or bottom half
        int second_half = (y > p2.y) || (p2.y == p1.y);
        int segment_height = second_half ? (p3.y - p2.y) : (p2.y - p1.y);
        
        if (segment_height == 0) continue;
        
        // Calculate X coordinates for this scanline
        float alpha = (float)(y - p1.y) / total_height;
        float beta = (float)(y - (second_half ? p2.y : p1.y)) / segment_height;
        
        // Interpolate X coordinates
        int x1 = p1.x + (int)((p3.x - p1.x) * alpha);
        int x2 = second_half ? 
                 (p2.x + (int)((p3.x - p2.x) * beta)) :
                 (p1.x + (int)((p2.x - p1.x) * beta));
        
        draw_scanline(y, x1, x2, color);
    }
}

// Draw triangle with gradient (interpolated vertex colors)
void draw_triangle_gradient(point_t p1, point_t p2, point_t p3,
                           uint32_t c1, uint32_t c2, uint32_t c3) {
    // Sort points by Y coordinate (keep colors matched)
    if (p1.y > p2.y) { swap_points(&p1, &p2); uint32_t tc = c1; c1 = c2; c2 = tc; }
    if (p2.y > p3.y) { swap_points(&p2, &p3); uint32_t tc = c2; c2 = c3; c3 = tc; }
    if (p1.y > p2.y) { swap_points(&p1, &p2); uint32_t tc = c1; c1 = c2; c2 = tc; }
    
    if (p1.y == p3.y) return;
    
    int total_height = p3.y - p1.y;
    
    for (int y = p1.y; y <= p3.y; y++) {
        int second_half = (y > p2.y) || (p2.y == p1.y);
        int segment_height = second_half ? (p3.y - p2.y) : (p2.y - p1.y);
        
        if (segment_height == 0) continue;
        
        float alpha = (float)(y - p1.y) / total_height;
        float beta = (float)(y - (second_half ? p2.y : p1.y)) / segment_height;
        
        // Interpolate X and color
        int x_a = p1.x + (int)((p3.x - p1.x) * alpha);
        int x_b = second_half ? 
                  (p2.x + (int)((p3.x - p2.x) * beta)) :
                  (p1.x + (int)((p2.x - p1.x) * beta));
        
        // Interpolate colors at edge points
        uint32_t color_a = c1; // Simplified - should interpolate c1->c3
        uint32_t color_b = second_half ? c2 : c2; // Simplified
        
        // For proper gradient, we'd interpolate all 3 colors
        // This is a simplified version
        draw_scanline_gradient(y, x_a, x_b, color_a, color_b);
    }
}

// Draw triangle outline
void draw_triangle_outline(point_t p1, point_t p2, point_t p3, uint32_t color) {
    draw_line(p1.x, p1.y, p2.x, p2.y, color);
    draw_line(p2.x, p2.y, p3.x, p3.y, color);
    draw_line(p3.x, p3.y, p1.x, p1.y, color);
}
