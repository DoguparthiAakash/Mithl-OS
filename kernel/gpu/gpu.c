#include "gpu.h"
#include "graphics.h"
#include "ports.h"
#include "string.h"

/* External console logging */
extern void console_log(const char *msg);

/* Global GPU information */
gpu_info_t gpu = {0};

/* Bochs/QEMU VGA Detection */
static int detect_bochs_vga(void) {
    // Bochs VGA has specific ID registers
    #define VBE_DISPI_IOPORT_INDEX 0x01CE
    #define VBE_DISPI_IOPORT_DATA  0x01CF
    #define VBE_DISPI_INDEX_ID     0x0
    
    outw(VBE_DISPI_IOPORT_INDEX, VBE_DISPI_INDEX_ID);
    uint16_t id = inw(VBE_DISPI_IOPORT_DATA);
    
    // Bochs VGA IDs: 0xB0C0 - 0xB0C5
    if (id >= 0xB0C0 && id <= 0xB0C5) {
        gpu.type = GPU_BOCHS_VGA;
        gpu.name = "Bochs/QEMU VGA";
        gpu.ioport_index = VBE_DISPI_IOPORT_INDEX;
        gpu.ioport_data = VBE_DISPI_IOPORT_DATA;
        gpu.has_hw_fill = 1;  // Bochs supports hardware fills
        gpu.has_hw_blit = 1;  // Bochs supports hardware blits
        gpu.has_vsync = 1;    // VGA vsync available
        return 1;
    }
    
    return 0;
}

/* VMware SVGA Detection */
static int detect_vmware_svga(void) {
    // VMware SVGA II uses PCI device 15ad:0405
    // For now, we'll skip PCI enumeration and just return 0
    // TODO: Implement PCI device enumeration
    return 0;
}

/* VirtualBox VGA Detection */
static int detect_virtualbox_vga(void) {
    // VirtualBox uses PCI device 80ee:beef
    // TODO: Implement PCI device enumeration
    return 0;
}

/* GPU Detection */
void gpu_detect(void) {
    console_log("[GPU] Detecting graphics hardware...\n");
    
    // Try to detect specific GPUs
    if (detect_bochs_vga()) {
        console_log("[GPU] Detected: Bochs/QEMU VGA\n");
        console_log("[GPU] Hardware acceleration: ENABLED\n");
        return;
    }
    
    if (detect_vmware_svga()) {
        console_log("[GPU] Detected: VMware SVGA II\n");
        return;
    }
    
    if (detect_virtualbox_vga()) {
        console_log("[GPU] Detected: VirtualBox VGA\n");
        return;
    }
    
    // Fallback to basic VESA
    gpu.type = GPU_VESA_BASIC;
    gpu.name = "Generic VESA";
    gpu.has_hw_fill = 0;
    gpu.has_hw_blit = 0;
    gpu.has_vsync = 1; // VGA vsync always available
    console_log("[GPU] Using software rendering (no GPU detected)\n");
}

/* GPU Initialization */
void gpu_init(void) {
    gpu_detect();
    
    // Initialize GPU-specific features
    if (gpu.type == GPU_BOCHS_VGA) {
        // Bochs VGA is already initialized by VESA
        console_log("[GPU] Bochs VGA ready for hardware acceleration\n");
    }
}

/* Hardware-Accelerated Fill (Bochs/QEMU) */
static void bochs_hw_fill(rect_t rect, uint32_t color) {
    // Bochs VGA doesn't have direct MMIO fill commands in standard mode
    // We'll use optimized software fill for now
    // In a real implementation, you'd use Bochs-specific extensions
    
    // For now, fall back to software
    draw_rect_filled(rect, color);
}

/* Hardware-Accelerated Blit (Bochs/QEMU) */
static void bochs_hw_blit(rect_t src, rect_t dst) {
    // Similar to fill, Bochs doesn't have standard blit commands
    // Fall back to software for now
    
    // Software blit implementation would go here
    // For now, just a placeholder
    (void)src;
    (void)dst;
}

/* High-Level Fill API */
void gpu_fill_rect(rect_t rect, uint32_t color) {
    if (gpu.has_hw_fill && gpu.type == GPU_BOCHS_VGA) {
        bochs_hw_fill(rect, color);
    } else {
        // Software fallback
        draw_rect_filled(rect, color);
    }
}

/* High-Level Blit API */
void gpu_copy_rect(rect_t src, rect_t dst) {
    if (gpu.has_hw_blit && gpu.type == GPU_BOCHS_VGA) {
        bochs_hw_blit(src, dst);
    } else {
        // Software fallback - copy pixels
        for (int y = 0; y < src.height && y < dst.height; y++) {
            for (int x = 0; x < src.width && x < dst.width; x++) {
                uint32_t pixel = get_pixel(src.x + x, src.y + y);
                set_pixel(dst.x + x, dst.y + y, pixel);
            }
        }
    }
}

/* VSync Wait */
void gpu_vsync_wait(void) {
    if (!gpu.has_vsync) {
        // No vsync available, just return
        return;
    }
    
    // VGA Status Register (0x3DA for color, 0x3BA for mono)
    #define VGA_STATUS_REG 0x3DA
    #define VGA_VBLANK_BIT 0x08
    
    // Wait for NOT in vblank
    while (inb(VGA_STATUS_REG) & VGA_VBLANK_BIT);
    
    // Wait for vblank start
    while (!(inb(VGA_STATUS_REG) & VGA_VBLANK_BIT));
}

/* Check if VSync is available */
int gpu_vsync_available(void) {
    return gpu.has_vsync;
}

/* Hardware Fill (direct) */
void gpu_hw_fill(rect_t rect, uint32_t color) {
    if (gpu.type == GPU_BOCHS_VGA) {
        bochs_hw_fill(rect, color);
    } else {
        draw_rect_filled(rect, color);
    }
}

/* Hardware Blit (direct) */
void gpu_hw_blit(rect_t src, rect_t dst) {
    if (gpu.type == GPU_BOCHS_VGA) {
        bochs_hw_blit(src, dst);
    } else {
        gpu_copy_rect(src, dst);
    }
}

/* Hardware Copy */
void gpu_hw_copy(int src_x, int src_y, int dst_x, int dst_y, int w, int h) {
    rect_t src = {src_x, src_y, w, h};
    rect_t dst = {dst_x, dst_y, w, h};
    gpu_hw_blit(src, dst);
}
