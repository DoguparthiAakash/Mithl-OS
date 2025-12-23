#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <time.h>
#include "drm_backend.h"
#include "window_manager.h"
#include "input.h"

// Hardcoded Cursor (10x10 red box for visibility)
static void draw_cursor(uint32_t *fb, int width, int height, int mx, int my) {
    uint32_t color = 0xFFFF0000; // Red
    for (int y = 0; y < 10; y++) {
        for (int x = 0; x < 10; x++) {
            int px = mx + x;
            int py = my + y;
            if (px >= 0 && px < width && py >= 0 && py < height) {
                fb[py * width + px] = color;
            }
        }
    }
}

int main(int argc, char **argv) {
    printf("[Compositor] Integrating Desktop Environment...\n");
    
    // 1. Init Graphics (DRM)
    mithl_drm_context_t *drm = drm_init();
    if (!drm) {
        fprintf(stderr, "[Compositor] DRM Init Failed\n");
        return 1;
    }
    printf("[Compositor] Graphics Ready: %dx%d\n", drm->mode.hdisplay, drm->mode.vdisplay);
    
    // 2. Init Window Manager
    wm_manager_t *wm = wm_init(drm->mode.hdisplay, drm->mode.vdisplay);
    if (!wm) {
        fprintf(stderr, "[Compositor] WM Init Failed\n");
        return 1;
    }
    
    // Create some test windows
    wm_create_window(wm, 100, 100, 400, 300, "Terminal");
    wm_create_window(wm, 600, 200, 300, 200, "Settings");
    
    // 3. Init Input
    input_state_t *input = input_init(drm->mode.hdisplay, drm->mode.vdisplay);
    if (!input) {
        fprintf(stderr, "[Compositor] Input Init Failed (Continuing without input)\n");
    } else {
        printf("[Compositor] Input Ready\n");
    }
    
    printf("[Compositor] Entering Main Loop\n");
    
    // Main Loop
    while (1) {
        // A. Poll Input
        if (input) {
            input_poll(input);
            
            // Handle interactions (very basic: verify cursor moves)
            // Ideally pass input events to WM here
        }
        
        // B. Get Buffer
        uint32_t *fb = (uint32_t*)drm_get_buffer(drm);
        if (!fb) break;
        
        // C. Render WM
        wm_render(wm, fb);
        
        // D. Draw Cursor (Overlay)
        if (input) {
            draw_cursor(fb, wm->width, wm->height, input->mouse_x, input->mouse_y);
        }
        
        // E. Flip
        drm_page_flip(drm);
        
        // yield
        // usleep(1000); 
    }
    
    return 0;
}
