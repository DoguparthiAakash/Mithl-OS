// mouse.c
#include "mouse.h"
#include "ports.h"
#include "graphics.h" // For screen dimensions

#define PS2_DATA_PORT 0x60
#define PS2_STATUS_PORT 0x64

static mouse_state_t ms = {0};
static int16_t cursor_x = 512, cursor_y = 384; // Center of 1024x768

static uint8_t mouse_cycle = 0;
static uint8_t mouse_byte[4]; // Buffer for up to 4 bytes
static uint8_t previous_buttons = 0;
static uint8_t mouse_packet_size = 3; // Default to standard PS/2

// Helper functions for PS/2 communication
static void mouse_wait_input()
{
    uint32_t timeout = 100000;
    while (timeout--)
    {
        if (!(inb(PS2_STATUS_PORT) & 0x02))
            return;
    }
}

static void mouse_wait_output()
{
    uint32_t timeout = 100000;
    while (timeout--)
    {
        if (inb(PS2_STATUS_PORT) & 0x01)
            return;
    }
}

static void mouse_write(uint8_t value)
{
    mouse_wait_input();
    outb(PS2_STATUS_PORT, 0xD4);
    mouse_wait_input();
    outb(PS2_DATA_PORT, value);
}

static uint8_t mouse_read()
{
    mouse_wait_output();
    return inb(PS2_DATA_PORT);
}

// Helper to set sample rate
static void mouse_set_sample_rate(uint8_t rate) {
    mouse_write(0xF3);
    mouse_read(); // ACK
    mouse_write(rate);
    mouse_read(); // ACK
}

void mouse_init(void)
{
    // Initialize state first
    ms.x = cursor_x;
    ms.y = cursor_y;
    ms.z = 0;
    ms.buttons = 0;
    ms.left_clicked = ms.right_clicked = ms.middle_clicked = 0;
    previous_buttons = 0;
    mouse_cycle = 0;
    mouse_packet_size = 3; // Strict 3-byte PS/2

    // Enable auxiliary mouse device
    mouse_wait_input();
    outb(PS2_STATUS_PORT, 0xA8);

    // Get current configuration
    mouse_wait_input();
    outb(PS2_STATUS_PORT, 0x20);
    mouse_wait_output();
    uint8_t status = inb(PS2_DATA_PORT);

    // Disable interrupt (bit 1) and disable clock line (bit 5)
    status &= ~0x02; // Disable IRQ 12 (Use Polling)
    status &= ~0x20; // Enable Mouse Port

    // Write back configuration
    mouse_wait_input();
    outb(PS2_STATUS_PORT, 0x60);
    mouse_wait_input();
    outb(PS2_DATA_PORT, status);

    // Reset mouse
    mouse_write(0xF6);
    mouse_read(); // ACK

    // Enable Data Reporting
    mouse_write(0xF4);
    mouse_read(); // ACK
    
    // Try to enable IntelliMouse (Magic Knock 1: 200, 100, 80)
    mouse_set_sample_rate(200);
    mouse_set_sample_rate(100);
    mouse_set_sample_rate(80);
    
    // Check ID
    mouse_write(0xF2);
    mouse_read(); // ACK
    uint8_t id = mouse_read();
    
    if (id == 3) {
        // Try Magic Knock 2 (200, 200, 80) for 5-button mouse (ID 4)
        mouse_set_sample_rate(200);
        mouse_set_sample_rate(200);
        mouse_set_sample_rate(80);
        
        mouse_write(0xF2);
        mouse_read(); // ACK
        uint8_t id2 = mouse_read();
        
        if (id2 == 4) {
            id = 4;
        }
    }
    
    if (id == 3 || id == 4) {
        mouse_packet_size = 4;
        // Set final sample rate
        mouse_set_sample_rate(200);
    } else {
        mouse_packet_size = 3; // Fallback to standard
    }

    // Re-enable data reporting
    mouse_write(0xF4);
    mouse_read();
}

int mouse_poll(void)
{
    uint8_t status = inb(PS2_STATUS_PORT);
    // Strict Mouse Data Check (Bit 5 must be set)
    if ((status & 0x21) != 0x21) return 0;

    uint8_t byte = inb(PS2_DATA_PORT);

    // Sync Check: First byte MUST have Bit 3 set (0x08).
    if (mouse_cycle == 0) {
        if (!(byte & 0x08)) {
            return 0; // Out of sync
        }
    }

    mouse_byte[mouse_cycle] = byte;
    mouse_cycle++;

    if (mouse_cycle >= mouse_packet_size)
    {
        mouse_cycle = 0;

        uint8_t buttons = mouse_byte[0] & 0x07;

        // Position
        int16_t x_delta = mouse_byte[1];
        int16_t y_delta = mouse_byte[2];
        
        // Byte 0 bits: 4=X sign, 5=Y sign
        if (mouse_byte[0] & 0x10) x_delta |= 0xFF00;
        if (mouse_byte[0] & 0x20) y_delta |= 0xFF00;
        
        // Sensitivity Multiplier (4x for speed)
        x_delta *= 4;
        y_delta *= 4;
        
        // Overflow check
        if ((mouse_byte[0] & 0xC0) != 0) {
            x_delta = 0;
            y_delta = 0;
        }

        // Z-axis
        int8_t z_delta = 0;
        if (mouse_packet_size == 4) {
            if (mouse_byte[3] & 0x0F) {
                 z_delta = (int8_t)(mouse_byte[3] & 0x0F);
                 if (z_delta & 0x08) z_delta |= 0xF0; // Sign extend
            }
        }
        
        // Updates
        cursor_x += x_delta;
        cursor_y -= y_delta; // Y inverted
        ms.z += z_delta;

        // Clamp
        if (cursor_x < 0) cursor_x = 0;
        if (cursor_x >= (int16_t)fb.width) cursor_x = (int16_t)fb.width - 1;
        if (cursor_y < 0) cursor_y = 0;
        if (cursor_y >= (int16_t)fb.height) cursor_y = (int16_t)fb.height - 1;

        ms.x = cursor_x;
        ms.y = cursor_y;
        ms.buttons = buttons;
        ms.left_clicked   = (buttons & MOUSE_LEFT_BUTTON) ? 1 : 0;
        ms.right_clicked  = (buttons & MOUSE_RIGHT_BUTTON) ? 1 : 0;
        ms.middle_clicked = (buttons & MOUSE_MIDDLE_BUTTON) ? 1 : 0;

        // Push Event
        #include "input.h"
        mouse_event_t me;
        me.x = ms.x;
        me.y = ms.y;
        me.rel_x = x_delta;
        me.rel_y = y_delta;
        me.rel_z = z_delta;
        me.z = ms.z; 
        me.action = MOUSE_MOTION;
        me.pressed = ms.left_clicked;
        
        input_add_mouse_event(&me);
        
        if (ms.left_clicked && !(previous_buttons & MOUSE_LEFT_BUTTON)) {
             me.action = MOUSE_LEFT; me.pressed = 1; input_add_mouse_event(&me);
        } else if (!ms.left_clicked && (previous_buttons & MOUSE_LEFT_BUTTON)) {
             me.action = MOUSE_LEFT; me.pressed = 0; input_add_mouse_event(&me);
        }

        if (ms.right_clicked && !(previous_buttons & MOUSE_RIGHT_BUTTON)) {
             me.action = MOUSE_RIGHT; me.pressed = 1; input_add_mouse_event(&me);
        } else if (!ms.right_clicked && (previous_buttons & MOUSE_RIGHT_BUTTON)) {
             me.action = MOUSE_RIGHT; me.pressed = 0; input_add_mouse_event(&me);
        }

        if (ms.middle_clicked && !(previous_buttons & MOUSE_MIDDLE_BUTTON)) {
             me.action = MOUSE_MIDDLE; me.pressed = 1; input_add_mouse_event(&me);
        } else if (!ms.middle_clicked && (previous_buttons & MOUSE_MIDDLE_BUTTON)) {
             me.action = MOUSE_MIDDLE; me.pressed = 0; input_add_mouse_event(&me);
        }

        previous_buttons = buttons;
        return 1; // Packet Complete
    }
    return 1; // Partial
}

mouse_state_t *mouse_get_state(void)
{
    return &ms;
}

void mouse_set_position(int16_t x, int16_t y)
{
    // Clamp to screen boundaries
    if (x < 0)
        x = 0;
    if (x >= (int16_t)fb.width)
        x = (int16_t)fb.width - 1;
    if (y < 0)
        y = 0;
    if (y >= (int16_t)fb.height)
        y = (int16_t)fb.height - 1;

    ms.x = x;
    ms.y = y;
    cursor_x = x;
    cursor_y = y;
}

void mouse_get_position(int16_t *x, int16_t *y)
{
    *x = ms.x;
    *y = ms.y;
}

uint8_t mouse_is_button_pressed(uint8_t button)
{
    return ms.buttons & button;
}
