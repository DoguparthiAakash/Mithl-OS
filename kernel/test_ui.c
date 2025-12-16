#include "gui.h"
#include "desktop.h"
#include "terminal.h"
#include "file_manager.h"
#include "input.h"
#include "ports.h"

// Serial output helper
static void serial_write_char(char c) {
    // Wait for transmit empty? Just write for now, QEMU handles it fast.
    outb(0x3F8, c);
}

static void serial_print(const char *str) {
    while (*str) {
        serial_write_char(*str++);
    }
}

// Basic assertion
static void assert_true(int condition, const char *msg) {
    if (condition) {
        serial_print("[PASS] ");
        serial_print(msg);
        serial_print("\n");
    } else {
        serial_print("[FAIL] ");
        serial_print(msg);
        serial_print("\n");
    }
}

void test_ui_clicks(void) {
    serial_print("\n=== STARTING UI LOGIC TEST ===\n");
    
    // 1. Verify Root Exists
    assert_true(gui_mgr.root != 0, "GUI Root exists");
    
    // 2. Simulate Click on "Computers" (28, 20) -> (92, 100)
    // Click center of icon: 28+32=60, 20+40=60
    
    // Check hit test
    gui_element_t *target = gui_find_element_at(60, 60);
    assert_true(target == gui_mgr.root, "Click on desktop icon hits Root (Desktop)");
    
    if (target == gui_mgr.root) {
        // Dispatch Event
        gui_event_t click;
        click.type = GUI_EVENT_MOUSE_DOWN;
        click.mouse.pos.x = 60;
        click.mouse.pos.y = 60;
        
        // Call handler directly to verify logic
        // We can't easily check side effects unless we check global state.
        // But running it without crash is a good sign.
        if (target->event_handler) {
             target->event_handler(target, &click);
             serial_print("[INFO] Computer Icon Click Handled\n");
        }
    }
    
    // 3. Simulate Click on "Documents" (28, 110)
    // Center: 60, 150
    target = gui_find_element_at(60, 150);
    assert_true(target == gui_mgr.root, "Click on Documents hits Root");
    
    if (target == gui_mgr.root && target->event_handler) {
         gui_event_t click;
         click.type = GUI_EVENT_MOUSE_DOWN;
         click.mouse.pos.x = 60;
         click.mouse.pos.y = 150;
         target->event_handler(target, &click);
         serial_print("[INFO] Documents Icon Click Handled\n");
    }
    
    serial_print("=== UI TEST COMPLETE ===\n\n");
}
