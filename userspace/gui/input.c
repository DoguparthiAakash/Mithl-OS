#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <libinput.h>
#include <libudev.h>
#include "input.h"

static int open_restricted(const char *path, int flags, void *user_data) {
    int fd = open(path, flags);
    return fd < 0 ? -errno : fd;
}

static void close_restricted(int fd, void *user_data) {
    close(fd);
}

static const struct libinput_interface interface = {
    .open_restricted = open_restricted,
    .close_restricted = close_restricted,
};

static int g_width, g_height;

input_state_t* input_init(int width, int height) {
    g_width = width;
    g_height = height;
    
    input_state_t *state = calloc(1, sizeof(input_state_t));
    state->mouse_x = width / 2;
    state->mouse_y = height / 2;
    
    state->udev = udev_new();
    if (!state->udev) {
        fprintf(stderr, "Failed to initialize udev\n");
        free(state);
        return NULL;
    }
    
    state->li = libinput_udev_create_context(&interface, NULL, state->udev);
    if (!state->li) {
        fprintf(stderr, "Failed to initialize libinput\n");
        udev_unref(state->udev);
        free(state);
        return NULL;
    }
    
    libinput_udev_assign_seat(state->li, "seat0");
    return state;
}

void input_poll(input_state_t *input) {
    libinput_dispatch(input->li);
    
    struct libinput_event *ev;
    while ((ev = libinput_get_event(input->li))) {
        enum libinput_event_type type = libinput_event_get_type(ev);
        
        switch (type) {
            case LIBINPUT_EVENT_POINTER_MOTION: {
                struct libinput_event_pointer *p = libinput_event_get_pointer_event(ev);
                double dx = libinput_event_pointer_get_dx(p);
                double dy = libinput_event_pointer_get_dy(p);
                
                input->mouse_x += (int)dx;
                input->mouse_y += (int)dy;
                
                // Clamp
                if (input->mouse_x < 0) input->mouse_x = 0;
                if (input->mouse_y < 0) input->mouse_y = 0;
                if (input->mouse_x >= g_width) input->mouse_x = g_width - 1;
                if (input->mouse_y >= g_height) input->mouse_y = g_height - 1;
                break;
            }
            case LIBINPUT_EVENT_POINTER_BUTTON: {
                struct libinput_event_pointer *p = libinput_event_get_pointer_event(ev);
                uint32_t btn = libinput_event_pointer_get_button(p);
                uint32_t state = libinput_event_pointer_get_button_state(p);
                
                if (btn == 272) input->left_btn = (state == LIBINPUT_BUTTON_STATE_PRESSED); // Left
                if (btn == 273) input->right_btn = (state == LIBINPUT_BUTTON_STATE_PRESSED); // Right
                break;
            }
            case LIBINPUT_EVENT_KEYBOARD_KEY: {
                struct libinput_event_keyboard *k = libinput_event_get_keyboard_event(ev);
                input->last_key = libinput_event_keyboard_get_key(k);
                input->key_state = libinput_event_keyboard_get_key_state(k);
                break;
            }
            default:
                break;
        }
        
        libinput_event_destroy(ev);
    }
}

void input_cleanup(input_state_t *input) {
    libinput_unref(input->li);
    udev_unref(input->udev);
    free(input);
}
