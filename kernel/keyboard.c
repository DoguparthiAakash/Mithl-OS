#include "keyboard.h"
#include "ports.h"
#include "input.h"

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
static const uint8_t scancode_to_ascii_shifted[128] = {
    0, 0, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
    0, '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0,
    '*', 0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '-',
    0, 0, 0, '+', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

// Modifier State
static uint8_t mod_state = 0;

void keyboard_init(void)
{
    // Initialize keyboard state
    ks.key_pressed = 0;
    ks.key_code = 0;
    ks.ascii = 0;
    mod_state = 0;
}

// Extended flag
static int extended = 0;

// Serial Debug Helper
static void serial_log_byte(uint8_t b) {
    while ((inb(0x3F8 + 5) & 0x20) == 0);
    outb(0x3F8, b);
}
static void serial_log_str(const char* s) {
    while(*s) serial_log_byte(*s++);
}
static void serial_log_hex(uint8_t n) {
    char h[] = "0123456789ABCDEF";
    serial_log_byte(h[n >> 4]);
    serial_log_byte(h[n & 0xF]);
}

int keyboard_poll(void)
{
    uint8_t status = inb(KEYBOARD_STATUS_PORT);

    // Check if data is available (Bit 0 set) AND it is NOT mouse data (Bit 5 clear)
    if ((status & 0x21) == 0x01)
    {
        uint8_t scancode = inb(KEYBOARD_DATA_PORT);
        
        // DEBUG: Log every scancode
        serial_log_str("[KBD] Scancode: ");
        serial_log_hex(scancode);
        serial_log_str(extended ? " (EXT)\n" : "\n");
        
        if (scancode == 0xE0) {
            extended = 1;
            return 1;
        }
        
        int release = (scancode & 0x80) ? 1 : 0;
        uint8_t code = scancode & 0x7F;

        // Modifier Keycodes (Set 1)
        
        if (!extended) {
            if (code == 0x2A || code == 0x36) { // Shift
                if (release) mod_state &= ~KEY_MOD_SHIFT;
                else mod_state |= KEY_MOD_SHIFT;
            } else if (code == 0x1D) { // Ctrl
                if (release) mod_state &= ~KEY_MOD_CTRL;
                else mod_state |= KEY_MOD_CTRL;
            } else if (code == 0x38) { // Alt
                if (release) mod_state &= ~KEY_MOD_ALT;
                else mod_state |= KEY_MOD_ALT;
            } else if (code == 0x3A) { // CapsLock
                if (!release) { // Toggle on press
                     if (mod_state & KEY_MOD_CAPS) mod_state &= ~KEY_MOD_CAPS;
                     else mod_state |= KEY_MOD_CAPS;
                }
            }
        } else {
             // Extended Keys
             // Windows Key (LWin = 0x5B, RWin = 0x5C)
             // DEBUG: Log Win Key Detection
             if (code == 0x5B || code == 0x5C) {
                 serial_log_str("[KBD] Win Key Detected!\n");
                 if (release) mod_state &= ~KEY_MOD_META;
                 else mod_state |= KEY_MOD_META;
             }
             // Add other extended keys if needed
        }

        if (release)
        {
            ks.key_pressed = 0;
            ks.key_code = code;
            ks.ascii = 0;
        }
        else
        {
            // Key press
            ks.key_pressed = 1;
            ks.key_code = scancode;

            // Resolve ASCII
            uint8_t ascii = 0;
            if (!extended && code < 128) { // Normal map
                // Determine if we should use shifted map
                // Shift works on everything in shifted map
                // CapsLock only affects letters (approx. 0x10-0x19, 0x1E-0x26, 0x2C-0x32)
                
                int use_shift = (mod_state & KEY_MOD_SHIFT);
                int is_alpha = (code >= 0x10 && code <= 0x19) || // Q-P
                               (code >= 0x1E && code <= 0x26) || // A-L
                               (code >= 0x2C && code <= 0x32);   // Z-M
                               
                if (mod_state & KEY_MOD_CAPS && is_alpha) {
                    // Caps inverts shift for letters
                    use_shift = !use_shift;
                }
                
                if (use_shift) {
                    ascii = scancode_to_ascii_shifted[code];
                } else {
                    ascii = scancode_to_ascii[code];
                }
            }
            ks.ascii = ascii;
            
            // Post event to queue with modifiers
            input_add_key_event(code, ascii, mod_state, 0); // 0 = Press
        }
        
        // Reset extended unless it was E0 (handled above)
        if (scancode != 0xE0) extended = 0; 
        
        return 1;
    }

    return 0;
}

keyboard_state_t *keyboard_get_state(void)
{
    return &ks;
}