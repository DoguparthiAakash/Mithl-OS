#ifndef INPUT_H
#define INPUT_H

#include <libinput.h>
#include <stdbool.h>

typedef struct {
    int mouse_x;
    int mouse_y;
    bool left_btn;
    bool right_btn;
    
    // Keyboard
    uint32_t last_key;
    uint32_t key_state; // 0=release, 1=press
    
    struct libinput *li;
    struct udev *udev;
} input_state_t;

input_state_t* input_init(int screen_width, int screen_height);
void input_poll(input_state_t *input);
void input_cleanup(input_state_t *input);

#endif
