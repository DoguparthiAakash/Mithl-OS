#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include "drm_backend.h"

int main(int argc, char **argv) {
    printf("[Compositor] Starting DRM Backend...\n");
    
    mithl_drm_context_t *drm = drm_init();
    if (!drm) {
        fprintf(stderr, "[Compositor] Failed to initialize DRM backend\n");
        return 1;
    }
    
    printf("[Compositor] DRM Initialized. Entering main loop.\n");
    
    int frame = 0;
    while (1) {
        // 1. Get Buffer
        uint32_t *fb = (uint32_t*)drm_get_buffer(drm);
        if (!fb) {
            fprintf(stderr, "Failed to get buffer\n");
            break;
        }
        
        // 2. Draw (Simple Color Cycle)
        int width = drm->mode.hdisplay;
        int height = drm->mode.vdisplay;
        
        // Cycle colors: R -> G -> B every 60 frames
        uint32_t color = 0xFF0000; // Red
        if ((frame / 60) % 3 == 1) color = 0x00FF00; // Green
        else if ((frame / 60) % 3 == 2) color = 0x0000FF; // Blue
        
        // Fill screen
        for (int i = 0; i < width * height; i++) {
            fb[i] = color;
        }
        
        // Draw moving square for "liveness"
        int box_x = (frame * 5) % (width - 100);
        int box_y = (frame * 5) % (height - 100);
        for (int y = box_y; y < box_y + 100; y++) {
            for (int x = box_x; x < box_x + 100; x++) {
                fb[y * width + x] = 0xFFFFFF; // White box
            }
        }
        
        // 3. Flip
        drm_page_flip(drm);
        frame++;
        
        // Throttle (simulation)
        usleep(16000); 
    }
    
    drm_cleanup(drm);
    return 0;
}
