#ifndef MOUSE_H
#define MOUSE_H

// Define our own types for freestanding environment
typedef unsigned char uint8_t;
typedef signed char int8_t;
typedef signed short int16_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;


typedef unsigned char bool_t;

// Mouse buttons
#define MOUSE_LEFT_BUTTON 0x01
#define MOUSE_RIGHT_BUTTON 0x02
#define MOUSE_MIDDLE_BUTTON 0x04

// Mouse state
typedef struct
{
    int16_t x;
    int16_t y;
    int8_t z; // Scroll wheel
    uint8_t buttons;
    uint8_t left_clicked;   // 1 if left button is currently pressed, 0 otherwise
    uint8_t right_clicked;  // 1 if right button is currently pressed, 0 otherwise
    uint8_t middle_clicked; // 1 if middle button is currently pressed, 0 otherwise
} mouse_state_t;

// Function prototypes
void mouse_init(void);
int mouse_poll(void);
mouse_state_t *mouse_get_state(void);
void mouse_set_position(int16_t x, int16_t y);
void mouse_get_position(int16_t *x, int16_t *y);
uint8_t mouse_is_button_pressed(uint8_t button);

#endif // MOUSE_H