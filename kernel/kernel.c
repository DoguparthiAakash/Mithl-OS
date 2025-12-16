#include "keyboard.h"
#include "mouse.h"
#include "memory.h"
#include "ports.h"
#include "event.h"
//#include "window.h" // Removed to avoid conflict
#include "stdint.h"
#include "string.h"
#include "console.h"
//#include "apps.h"
//#include "colors.h" // Removed to use graphics.h colors
#include "kernel.h"
#include "vga.h"
#include "gdt.h"
#include "gui.h"
#include "input.h"
#include "idt.h"
#include "multiboot.h"
#include "filesystem.h"
#include "desktop.h"
#include "graphics.h"
#include "mm/pmm.h"

// Initialize inputs
static void init_input_devices(void)
{
    keyboard_init();
    mouse_init();
    // keyboard_state = keyboard_get_state(); // unused for now
    // mouse_state = mouse_get_state();       // unused for now
}

// static int strcmp(const char *str1, const char *str2) ... removed unused
// static void strcpy(char *dest, const char *src) ... removed unused
// static int snprintf(char *str, size_t size, const char *format, ...) ... removed unused

// Rust entry point
void rust_init(void);

// Wrapper for renderer
void renderer_draw_text(const char *text, point_t pos, uint32_t color) {
    draw_text(text, pos.x, pos.y, color, 12); // Default size 12
}

extern void call_constructors(void);
extern void call_destructors(void);

// Enable SSE (SIMD) to prevent Invalid Opcode exceptions from Rust/GCC code
void enable_sse(void) {
    uint32_t cr4;
    __asm__ volatile("mov %%cr4, %0" : "=r"(cr4));
    cr4 |= 0x200; // Bit 9: OSFXSR
    cr4 |= 0x400; // Bit 10: OSXMMEXCPT (Unmasked SIMD FP Exceptions)
    __asm__ volatile("mov %0, %%cr4" :: "r"(cr4));

    uint32_t cr0;
    __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 &= ~0x04; // Clear EM (Bit 2) - Emulation
    cr0 |= 0x02;  // Set MP (Bit 1) - Monitor Coprocessor
    cr0 |= 0x20;  // Set NE (Bit 5) - Native Exception
    __asm__ volatile("mov %0, %%cr0" :: "r"(cr0));
}

// Logging helper
void console_log(const char *msg) {
    console_write(msg); // Try VGA (might be invisible)
    serial_write(msg);  // Try Serial (reliable)
}

void kmain(uint32_t magic, multiboot_info_t* mbi)
{
    // Initialize Serial Port FIRST for debugging
    serial_init();
    serial_write("\n\n=== MITHL OS KERNEL BOOT ===\n");
    serial_write("[INFO] Serial Port Initialized.\n");

    // Initialize GDT first (essential for stable segments)
    gdt_init();
    console_log("[INFO] GDT Initialized.\n");
    
    // Enable SSE immediately (before any Rust/Float code runs)
    enable_sse();

    // Disable PIC interrupts immediately to run in polling mode
    i8259_disable();
    console_log("[INFO] PIC Disabled.\n");
    // === Initialize systems ===
    call_constructors();
    console_log("[INFO] Constructors Called.\n");
    idt_init(); // Initialize IDT to catch exceptions
    console_log("[INFO] IDT Initialized.\n");
    // Check Multiboot Magic
    if (magic != MULTIBOOT_MAGIC) {
        serial_write("[CRITICAL] Invalid Multiboot Magic!\n");
        // We can't proceed safely usually, but let's try to hang or print
        console_write("PANIC: Invalid Multiboot Magic!\n");
        for(;;) __asm__ volatile("hlt");
    }

    // Initialize Physical Memory Manager (Phase 1)
    pmm_init(mbi);
    console_log("[INFO] PMM Initialized.\n");

    // Initialize Virtual Memory Manager (Phase 2)
    void vmm_init(multiboot_info_t* mboot_info); // Prototype
    vmm_init(mbi);
    console_log("[INFO] VMM Initialized (Paging Enabled).\n");

    // Initialize Heap (Requires VMM/PMM)
    memory_init();
    console_log("[INFO] Memory Initialized.\n");
    
    // Initialize zRAM
    void zram_init(void);
    zram_init();
    console_log("[INFO] zRAM Initialized.\n");
    
    // Initialize Runtime (Requires Heap)
    rust_init(); 
    console_log("[INFO] Rust Initialized.\n");

    // Initialize Graphics
    if (mbi->flags & MULTIBOOT_FLAG_FB) {
        console_log("[INFO] Multiboot Graphics Available. Initializing VESA...\n");
        graphics_init(mbi->framebuffer_width, mbi->framebuffer_height, 
                      mbi->framebuffer_pitch, mbi->framebuffer_bpp, 
                      (void*)(uint32_t)mbi->framebuffer_addr);
        console_log("[INFO] VESA Graphics Initialized.\n");
        
        // Initialize GPU acceleration
        void gpu_init(void);
        gpu_init();
    } else {
        serial_write("[CRITICAL] No Framebuffer Flag!\n");
        console_write("PANIC: No Framebuffer Flag!\n");
        for(;;) __asm__ volatile("hlt");
    }

    // Initialize input devices
    init_input_devices();
    
    // Initialize VFS
    void ramfs_init_clean(void);
    ramfs_init_clean();
    // fs_init(); // Old dumb filesystem 
    
    // Show Boot Logo
    console_write("[INFO] Drawing Boot Logo...\n");
    draw_boot_logo();
    for (volatile int i = 0; i < 300000000; i++); // Moderate delay ~2-4s on QEMU
    // console_write("[INFO] Boot Logo Skipped (Debug Mode).\n");

    // Initialize GUI System
    console_write("[INFO] Init GUI...\n");
    gui_renderer_t renderer;
    renderer.draw_rect = draw_rect_filled;
    renderer.draw_text = renderer_draw_text;
    gui_init(mbi->framebuffer_width, mbi->framebuffer_height, &renderer);

    console_write("[INFO] Init Desktop...\n");
    desktop_init();

    // Initial Draw
    // Initial Draw
    console_log("[INFO] First Desktop Draw...\n");
    desktop_draw();
    graphics_swap_buffers(); 
    console_log("[INFO] Draw Complete. Entering Loop.\n");
    
    static int prev_x = -1, prev_y = -1;
    gui_mgr.needs_redraw = 0;

    // === Main event loop ===
    while (1)
    {
        // 1. Process Input
        input_poll();
        
        // 2. Mouse Logic
        mouse_state_t* m = mouse_get_state();
        int cur_x = m->x;
        int cur_y = m->y;
        
        // Use hardcoded hotspot values matching cursor_icon.h
        // We could include the header but hardcoding is safe if we match it.
        // CURSOR_HOT_X = 5, CURSOR_HOT_Y = 4
        int hot_x = 5;
        int hot_y = 4;
        
        // If mouse moved, we need to redraw TWO areas:
        // A. The OLD cursor position (to erase it / restore background)
        // B. The NEW cursor position (to draw it)
        if (cur_x != prev_x || cur_y != prev_y) {
             // Invalidate OLD cursor area (shifted by hotspot)
             if (prev_x != -1) {
                  gui_invalidate_rect((rect_t){prev_x - hot_x, prev_y - hot_y, 48, 48});
             }
             // Invalidate NEW cursor area (shifted by hotspot)
             gui_invalidate_rect((rect_t){cur_x - hot_x, cur_y - hot_y, 48, 48});
        }

        // 3. Update Desktop / GUI
        // This processes events and redraws DIRTY regions to the BACKBUFFER.
        // It does NOT present to screen yet.
        desktop_check_clock();
        desktop_update(); 

        // 4. Force global redraw if requested (e.g. wallpaper change)
        if (gui_mgr.needs_redraw && gui_mgr.dirty_rect.width == gui_mgr.screen_width) {
            desktop_draw(); // Full redraw to backbuffer
        }
        
        // 5. Unconditional Mouse Overlay ON BACKBUFFER
        // We draw the cursor on top of the backbuffer content we just updated.
        // Since we invalidated the cursor rects, they are included in dirty_rect.
        draw_cursor_icon(cur_x, cur_y);
        
        // 6. Present Frame (Efficient Swap)
        // This copies ONLY the dirty_rect from Backbuffer to Video Memory.
        // This is the key optimization: reducing bandwidth from 3MB/frame to ~10KB/frame (mouse only).
        gui_present();
        
        // 7. Update State
        prev_x = cur_x;
        prev_y = cur_y;
    }
}

void _Unwind_Resume(void) {
    while (1) { __asm__ volatile("hlt"); }
}