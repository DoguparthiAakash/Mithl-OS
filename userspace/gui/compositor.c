/*
 * Mithl-OS Hybrid Compositor
 * Runs on Linux Framebuffer (/dev/fb0)
 * Ports the visual style of Mithl-OS to Userspace
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

// --- Global Framebuffer State ---
int fbfd = 0;
struct fb_var_screeninfo vinfo;
struct fb_fix_screeninfo finfo;
long int screensize = 0;
char *fbp = 0;

// --- Colors ---
#define BG_COLOR 0x008080   // Teal
#define BAR_COLOR 0xCCFFFFFF // Translucent White
#define DOCK_COLOR 0x40FFFFFF

// --- Graphics Primitives ---

// Put pixel (32bpp ARGB)
void put_pixel(int x, int y, uint32_t color) {
    if (x < 0 || x >= (int)vinfo.xres || y < 0 || y >= (int)vinfo.yres) return;
    
    long int location = (x+vinfo.xoffset) * (vinfo.bits_per_pixel/8) +
                        (y+vinfo.yoffset) * finfo.line_length;
    
    // Assume 32bpp (BGRA or ARGB)
    // Linux FB usually BGRA on x86, but we'll write 32bit int
    *((uint32_t*)(fbp + location)) = color;
}

void draw_rect_filled(int x, int y, int w, int h, uint32_t color) {
    for (int j = 0; j < h; j++) {
        for (int i = 0; i < w; i++) {
            put_pixel(x+i, y+j, color);
        }
    }
}

// Simple 8x8 bitmap font (Mock)
void draw_char(char c, int x, int y, uint32_t color) {
    // Just a placeholder block for text since we don't have the font header handy
    // Real implementation would include font8x8.h
    draw_rect_filled(x, y, 6, 8, color); 
}

void draw_text(const char *str, int x, int y, uint32_t color) {
    while (*str) {
        draw_char(*str++, x, y, color);
        x += 8;
    }
}

// --- Desktop Components (Ported) ---

void draw_wallpaper() {
    // Fill Screen with Teal
    draw_rect_filled(0, 0, vinfo.xres, vinfo.yres, BG_COLOR);
}

void draw_top_bar() {
    // Height 24
    draw_rect_filled(0, 0, vinfo.xres, 24, BAR_COLOR);
    
    // Draw "Mithl" text (Mock)
    draw_text("Mithl OS", 20, 8, 0xFF000000);
    
    // Draw Clock
    time_t rawtime;
    struct tm * timeinfo;
    char buffer[80];

    time (&rawtime);
    timeinfo = localtime (&rawtime);
    strftime(buffer, 80, "%H:%M", timeinfo);
    
    draw_text(buffer, vinfo.xres - 60, 8, 0xFF000000);
}

void draw_dock() {
    int num_icons = 6;
    int icon_size = 48;
    int pad = 16;
    int dock_w = num_icons * (icon_size + pad) + pad;
    int dock_h = 70;
    int dock_x = (vinfo.xres - dock_w) / 2;
    int dock_y = vinfo.yres - dock_h - 10;
    
    // Draw Background
    draw_rect_filled(dock_x, dock_y, dock_w, dock_h, DOCK_COLOR);
    
    // Draw Generated Icons (Colored Boxes for now)
    int start_x = dock_x + pad;
    int yp = dock_y + (dock_h - icon_size) / 2;
    
    uint32_t colors[] = {0xFFFF0000, 0xFF00FF00, 0xFF0000FF, 0xFFFFFF00, 0xFF00FFFF, 0xFF808080};
    
    for (int i = 0; i < num_icons; i++) {
        draw_rect_filled(start_x + i*(icon_size+pad), yp, icon_size, icon_size, colors[i]);
    }
}

// --- Main Loop ---

int main() {
    printf("[Mithl GUI] Starting Hybrid Compositor...\n");
    
    // 1. Open Framebuffer
    fbfd = open("/dev/fb0", O_RDWR);
    if (fbfd == -1) {
        perror("Error: cannot open framebuffer device");
        // Fallback for non-linux env (e.g. testing)
        return 1;
    }
    
    // 2. Get Info
    if (ioctl(fbfd, FBIOGET_FSCREENINFO, &finfo) == -1) {
        perror("Error reading fixed information");
        return 1;
    }
    if (ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo) == -1) {
        perror("Error reading variable information");
        return 1;
    }
    
    printf("[Mithl GUI] Res: %dx%d, %dbpp\n", vinfo.xres, vinfo.yres, vinfo.bits_per_pixel);
    
    // 3. Map Memory
    screensize = vinfo.xres * vinfo.yres * vinfo.bits_per_pixel / 8;
    fbp = (char *)mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, 0);
    if ((long)fbp == -1) {
        perror("Error: failed to map framebuffer device to memory");
        return 1;
    }
    
    // 4. Draw Loop
    printf("[Mithl GUI] Entering Draw Loop (Ctrl+C to exit)...\n");
    while (1) {
        draw_wallpaper();
        draw_top_bar();
        draw_dock();
        
        // Sleep 1 sec (Clock update)
        sleep(1);
    }
    
    munmap(fbp, screensize);
    close(fbfd);
    return 0;
}
