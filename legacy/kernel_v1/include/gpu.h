#ifndef GPU_H
#define GPU_H

#include "types.h"
#include "graphics.h"

/* GPU Types */
typedef enum {
    GPU_NONE,
    GPU_VESA_BASIC,      // Basic VESA (no acceleration)
    GPU_VBE_AF,          // VBE/AF (Accelerated Functions)
    GPU_BOCHS_VGA,       // Bochs/QEMU VGA (has acceleration)
    GPU_VMWARE_SVGA,     // VMware SVGA II
    GPU_VIRTUALBOX_VGA   // VirtualBox Graphics Adapter
} gpu_type_t;

/* GPU Capabilities */
typedef struct {
    gpu_type_t type;
    const char* name;
    uint32_t vram_size;
    
    // Hardware capabilities
    int has_hw_blit;
    int has_hw_fill;
    int has_hw_triangle;
    int has_hw_alpha;
    int has_vsync;
    
    // MMIO/IO ports
    void* mmio_base;
    uint16_t ioport_index;
    uint16_t ioport_data;
} gpu_info_t;

/* Global GPU info */
extern gpu_info_t gpu;

/* GPU Detection & Initialization */
void gpu_detect(void);
void gpu_init(void);

/* Hardware-Accelerated Operations */
void gpu_hw_fill(rect_t rect, uint32_t color);
void gpu_hw_blit(rect_t src, rect_t dst);
void gpu_hw_copy(int src_x, int src_y, int dst_x, int dst_y, int w, int h);

/* High-Level API (auto-selects HW or SW) */
void gpu_fill_rect(rect_t rect, uint32_t color);
void gpu_copy_rect(rect_t src, rect_t dst);

/* VSync */
void gpu_vsync_wait(void);
int gpu_vsync_available(void);

#endif /* GPU_H */
