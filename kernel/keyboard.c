#include "keyboard.h"
#include "ports.h"

#define KEYBOARD_DATA_PORT 0x60
#define KEYBOARD_STATUS_PORT 0x64

static keyboard_state_t ks = {0};

// Scancode to ASCII mapping (simplified)
const uint8_t scancode_to_ascii[128] = {
    0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
    '*', 0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '-',
    0, 0, 0, '+', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

// Shifted scancode to ASCII mapping
/* Unused shifted mapping
static const uint8_t scancode_to_ascii_shifted[128] = {
    0, 0, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
    0, '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0,
    '*', 0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '-',
    0, 0, 0, '+', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
*/

void keyboard_init(void)
{
    // Initialize keyboard state
    ks.key_pressed = 0;
    ks.key_code = 0;
    ks.ascii = 0;
}

int keyboard_poll(void)
{
    uint8_t status = inb(KEYBOARD_STATUS_PORT);

    // Check if data is available
    // Check if data is available (Bit 0 set) AND it is NOT mouse data (Bit 5 clear)
    // 0x21 = 0010 0001. We want 0x01.
    // If bit 5 (0x20) is set, it's mouse data, so ignore it here.
    if ((status & 0x21) == 0x01)
    {
        uint8_t scancode = inb(KEYBOARD_DATA_PORT);

        // Handle key release (bit 7 set)
        if (scancode & 0x80)
        {
            ks.key_pressed = 0;
            ks.key_code = scancode & 0x7F;
            ks.ascii = 0;
        }
        else
        {
            // Key press
            ks.key_pressed = 1;
            ks.key_code = scancode;

            // Convert to ASCII (simplified - no shift state tracking)
            if (scancode < 128)
            {
                ks.ascii = scancode_to_ascii[scancode];
            }
            else
            {
                ks.ascii = 0;
            }
            
            // Post event to queue
            #include "input.h" // Ensure input.h is visible or prototyped
            // We pass scancode as keycode for now. 
            // In a real OS we might map to a standard keycode first.
            input_add_key_event(scancode, 0); // 0 = Press
        }

        return 1;
    }

    return 0;
}

keyboard_state_t *keyboard_get_state(void)
{
    return &ks;
}