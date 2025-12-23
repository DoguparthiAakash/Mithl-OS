#ifndef INPUT_H
#define INPUT_H

#include "types.h"
#include "gui.h" // For gui_event_t

// --- Keyboard Event Definitions ---
// Note: Keycode definitions would typically be more extensive
// --- Keyboard Event Definitions ---
typedef enum
{
    KEY_PRESS,
    KEY_RELEASE,
    KEY_HOLD,
    KEY_REPEAT
} key_action_t;

// Modifier Key Flags
#define KEY_MOD_SHIFT (1 << 0)
#define KEY_MOD_CTRL  (1 << 1)
#define KEY_MOD_ALT   (1 << 2)
#define KEY_MOD_CAPS  (1 << 3)

typedef struct
{
    uint16_t keycode;    // Key scan code
    uint8_t modifiers;   // Modifier state
    char ascii;          // Resolved ASCII character
    key_action_t action; // Event type
    uint32_t timestamp;  // Time of event
} key_event_t;

// --- Mouse Event Definitions ---
typedef enum
{
    MOUSE_LEFT,
    MOUSE_RIGHT,
    MOUSE_MIDDLE,
    MOUSE_SCROLL_UP,
    MOUSE_SCROLL_DOWN,
    MOUSE_MOTION
} mouse_action_t;

typedef struct
{
    int16_t x;             // Coordinate position
    int16_t y;             // Coordinate position
    int16_t rel_x;         // Relative motion
    int16_t rel_y;         // Relative motion
    int16_t rel_z;         // Scroll delta
    int8_t z;              // Absolute Z (legacy)
    mouse_action_t action; // Button/Motion
    uint8_t pressed;       // 1 = button pressed, 0 = released
    uint32_t timestamp;    // Time of event
} mouse_event_t;

// --- Input Queue Functions (Public API) ---

/**
 * @brief Initializes the input subsystem.
 */
void input_init(void);

/**
 * @brief Polls for raw keyboard and mouse events and posts them to the GUI event queue.
 */
// Add to queue (for drivers)
// Add to queue (for drivers)
void input_add_key_event(uint16_t keycode, char ascii, uint8_t modifiers, uint8_t action);
void input_add_mouse_event(mouse_event_t *event_data);

void input_poll(void);

#endif // INPUT_H