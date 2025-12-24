
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
#include "boot_adapter.h"
#include "filesystem.h"
#include "desktop.h"
#include "graphics.h"
#include "mm/pmm.h"
#include "process.h"
#include "vfs.h"
#include "apps/file_manager/file_manager.h"
#include "fs/fat32/fat32.h"

// Forward declaration for Doom
extern void DOOM_Start(void);  // Wrapper that calls I_Init() then D_DoomMain()
extern void doom_assign_window(gui_window_t *w);

// Safe wrapper to prevent multiple launches or reentry if single-threaded
static int doom_launched = 0;
void launch_doom_safe(void) {
    if (doom_launched) return;
    doom_launched = 1;
    console_write("[KERNEL] Launching Doom...\n");
    
    // 1. Create Window for Doom (Main Thread/GUI Thread)
    // 320x200 scaled by 2 = 640x400. Add Chrome.
    gui_window_t *w = gui_create_window("DOOM", 100, 100, 640, 450);
    w->base.flags = 0; // Visible
    
    // 2. Assign window to Doom INTERFACE (i_mithl.c)
    doom_assign_window(w);
    
    // 3. Spawn Doom as a background PROCESS/THREAD
    // This prevents blocking the kernel/GUI loop!
    process_create("Doom", DOOM_Start);  // Use DOOM_Start wrapper
}

void launch_file_manager_safe(void) {
    file_manager_show();
}

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

// Global Boot Info
boot_info_t boot_info;

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

void kmain(uint32_t magic, void* addr)
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

    // Parse Multiboot Info
    static boot_info_t boot_info;
    if (parse_multiboot(magic, addr, &boot_info) != 0) {
        serial_write("[CRITICAL] Invalid Multiboot Magic!\n");
        console_write("PANIC: Invalid Multiboot Magic!\n");
        for(;;) __asm__ volatile("hlt");
    }

    // Pass ACPI RSDP (if present) to ACPI driver
    // This prevents dangerous logical memory scans in UEFI
    #include "acpi.h"
    if (boot_info.acpi_rsdp != 0) {
        acpi_set_rsdp(boot_info.acpi_rsdp);
        console_log("[INFO] ACPI RSDP set from Multiboot.\n");
    } else {
        console_log("[INFO] No ACPI RSDP in Multiboot. Will scan (risky in UEFI).\n");
    }

    // --- Memory Manager ---

    // Initialize Physical Memory Manager (Phase 1)
    pmm_init(&boot_info);
    console_log("[INFO] PMM Initialized.\n");

    // Initialize Virtual Memory Manager (Phase 2)
    void vmm_init(boot_info_t* boot_info); // Prototype
    vmm_init(&boot_info);
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
    if (boot_info.framebuffer.addr != 0) {
        console_log("[INFO] Multiboot Graphics Available. Initializing VESA...\n");
        graphics_init(boot_info.framebuffer.width, boot_info.framebuffer.height, 
                      boot_info.framebuffer.pitch, boot_info.framebuffer.bpp, 
                      (void*)(uint32_t)boot_info.framebuffer.addr);
        console_log("[INFO] VESA Graphics Initialized.\n");
        
        // Initialize GPU acceleration
        void gpu_init(void);
        gpu_init();
    } else {
        serial_write("[CRITICAL] No Framebuffer Found!\n");
        console_write("PANIC: No Framebuffer Found!\n");
        for(;;) __asm__ volatile("hlt");
    }
    
    // Pass framebuffer info to GUI later if needed, but gui_init uses values passed to it.
    // However, kmain calls gui_init using mbi->framebuffer_width... update that too.
    int fb_width = boot_info.framebuffer.width;
    int fb_height = boot_info.framebuffer.height;

    // Initialize input devices
    init_input_devices();
    
    // Initialize VFS and RamFS
    vfs_init();
    void ramfs_init_clean(void);
    ramfs_init_clean();
    
    // Load Modules (e.g. Doom WAD)
    extern void ramfs_load_modules(boot_info_t *info);
    ramfs_load_modules(&boot_info);
    // Initialize FAT32
    serial_write("[KERNEL] Initializing FAT32 (Drive 0, LBA 0)...\n");
    fat32_init(0);
    fs_node_t *hdd = fat32_mount();
    if (hdd) {
        strcpy(hdd->name, "hdd");
        ramfs_add_child(fs_root, hdd);
        serial_write("[KERNEL] Mounted FAT32 at /hdd\n");
    } else {
         serial_write("[KERNEL] Failed to mount FAT32\n");
    }
    
    // fs_init(); // Old dumb filesystem 
    
    // Initialize PIT (100Hz)
    void pit_init(uint32_t frequency);
    pit_init(100);
    
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
    gui_init(fb_width, fb_height, &renderer);

    console_write("[INFO] Init Desktop...\n");
    desktop_init();

    // Initial Draw
    console_log("[INFO] First Desktop Draw...\n");
    desktop_draw(); // Draws to Backbuffer
    
    // Fade in from Black to the Desktop we just drew
    graphics_fade_in();
    
    console_log("[INFO] Draw Complete. Entering Loop.\n");
    
    // Run HAL tests (DISABLED - causes crash in QEMU without proper CPU flags)
    // extern void hal_run_all_tests(void);
    // hal_run_all_tests();
    
    static int prev_x = -1, prev_y = -1;
    gui_mgr.needs_redraw = 0;

    // Initialize Process Manager
    process_init_main_thread();
    
    // Launch Userspace Hello App (The "Daily Driver" test)
    // Assumes ramfs loaded it at /hello.elf
    // process_create_elf("Hello", "/hello.elf");
    process_create_elf("Calculator", "/calculator.elf");
    
    // Enable Interrupts (PIT will drive preemption)
    asm volatile("sti");
    
    // Mouse Click Debounce State
    static int handled_click = 0;

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
        int hot_x = 5;
        int hot_y = 4;
        
        // DOCK INTERCEPTOR REMOVED - Handled by Compositor now


        // If mouse moved, we need to redraw TWO areas:
        // A. The OLD cursor position (to erase it / restore background)
        // B. The NEW cursor position (to draw it)
        int mouse_moved = 0;
        if (cur_x != prev_x || cur_y != prev_y) {
             // Invalidate OLD cursor area (shifted by hotspot)
             if (prev_x != -1) {
                  gui_invalidate_rect((rect_t){prev_x - hot_x, prev_y - hot_y, 48, 48});
             }
             // Invalidate NEW cursor area (shifted by hotspot)
             gui_invalidate_rect((rect_t){cur_x - hot_x, cur_y - hot_y, 48, 48});
             mouse_moved = 1;
        }

        // 3. Update Desktop / GUI
        // OPTIMIZATION: Skip desktop_update when idle
        // For now, always update to ensure keyboard input is visible
        // TODO: Add proper keyboard event detection
        int needs_update = 1; // Always update for now (fixes terminal input)
        
        if (needs_update) {
            desktop_check_clock();
            desktop_update(); 
        }

        // 4. Force global redraw if requested (e.g. wallpaper change)
        if (gui_mgr.needs_redraw && gui_mgr.dirty_rect.width == gui_mgr.screen_width) {
            desktop_draw(); // Full redraw to backbuffer
        }
        
        // 5. Unconditional Mouse Overlay ON BACKBUFFER (only if we're updating)
        // We draw the cursor on top of the backbuffer content we just updated.
        // Since we invalidated the cursor rects, they are included in dirty_rect.
        if (needs_update) {
            draw_cursor_icon(cur_x, cur_y);
        
            // 6. Present Frame (Efficient Swap)
            // This copies ONLY the dirty_rect from Backbuffer to Video Memory.
            // This is the key optimization: reducing bandwidth from 3MB/frame to ~10KB/frame (mouse only).
            gui_present();
        }
        
        // 7. Update State
        prev_x = cur_x;
        prev_y = cur_y;
    }
}

void _Unwind_Resume(void) {
    while (1) { __asm__ volatile("hlt"); }
}