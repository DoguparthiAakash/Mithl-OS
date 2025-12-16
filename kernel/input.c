#include "input.h"
#include "keyboard.h" // Assuming you have this driver
#include "mouse.h"    // Assuming you have this driver
#include "gui.h"      // For gui_post_event

// Placeholder for memory allocation
#include "memory.h"

// Define input-related functions
// input_init is defined in memory.c

void input_poll(void)
{
    // Poll hardware
    // Poll hardware aggressively to drain buffer
    // Loop max 10 times per frame to clear queue without hanging
    // Poll hardware aggressively to drain buffer
    // Loop max 100 times per frame to clear queue (3-4 bytes per packet * multiple packets)
    // mouse_poll returns 1 if it read a byte, 0 if empty.
    int max_cycles = 100;
    while (max_cycles > 0) {
         int activity = 0;
         if (mouse_poll()) activity = 1;
         if (keyboard_poll()) activity = 1;
         
         if (!activity) break; // Buffers empty
         max_cycles--;
    }

    // Process keyboard events
    if (keyboard_event_ready())
    {
        key_event_t *ke = receive_key_event();
        if (ke)
        {
            gui_event_t *gui_ke = (gui_event_t *)memory_alloc(sizeof(gui_event_t));
            if (gui_ke)
            {
                // Populate the GUI event from the keyboard event
                gui_ke->type = GUI_EVENT_KEY_PRESS; // Assuming a single type for now
                gui_ke->keyboard.key = (char)ke->keycode; // Cast or conversion needed
                gui_post_event(gui_ke);
            }
            // You may need to call free_key_event depending on your driver's design
        }
    }

    // Process mouse events
    if (mouse_event_ready())
    {
        mouse_event_t *me = receive_mouse_event();
        if (me)
        {
            gui_event_t *gui_me = (gui_event_t *)memory_alloc(sizeof(gui_event_t));
            if (gui_me)
            {
                int valid_event = 0;
                // Populate the GUI event from the mouse event
                if (me->action == MOUSE_MOTION)
                {
                    gui_me->type = GUI_EVENT_MOUSE_MOVE;
                    valid_event = 1;
                }
                else if (me->action == MOUSE_LEFT)
                {
                    gui_me->type = me->pressed ? GUI_EVENT_MOUSE_DOWN : GUI_EVENT_MOUSE_UP;
                    gui_me->mouse.button = 1;
                    valid_event = 1;
                }
                else if (me->action == MOUSE_RIGHT)
                {
                    gui_me->type = me->pressed ? GUI_EVENT_MOUSE_DOWN : GUI_EVENT_MOUSE_UP;
                    gui_me->mouse.button = 2;
                    valid_event = 1;
                }
                
                if (valid_event) {
                    gui_me->mouse.pos.x = me->x;
                    gui_me->mouse.pos.y = me->y;
                    gui_me->mouse.scroll_delta = 0; // Default
                    gui_post_event(gui_me);
                } 
                
                // Separate Check for Scroll (since action might be MOTION but has Z)
                if (me->rel_z != 0) {
                     gui_event_t *gui_scroll = (gui_event_t *)memory_alloc(sizeof(gui_event_t));
                     if (gui_scroll) {
                         gui_scroll->type = GUI_EVENT_MOUSE_SCROLL;
                         gui_scroll->mouse.pos.x = me->x;
                         gui_scroll->mouse.pos.y = me->y;
                         gui_scroll->mouse.scroll_delta = me->rel_z;
                         gui_post_event(gui_scroll);
                     }
                }

                if (!valid_event) {
                    memory_free(gui_me);
                }
            }
            // You may need to call free_mouse_event depending on your driver's design
        }
    }
}