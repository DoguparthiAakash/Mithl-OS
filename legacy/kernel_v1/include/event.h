#ifndef EVENT_H
#define EVENT_H

#include "types.h"

// Event types
typedef enum
{
    EVENT_KEY_PRESS,
    EVENT_KEY_RELEASE,
    EVENT_MOUSE_MOVE,
    EVENT_MOUSE_BUTTON
} event_type_t;

// Event structure
typedef struct
{
    event_type_t type;
    union
    {
        struct
        {
            uint8_t keycode;
        } key;
        struct
        {
            int16_t x, y;
            uint8_t button;
        } mouse;
    } data;
} event_t;

// Event queue structure
#define MAX_EVENTS 64
typedef struct
{
    event_t events[MAX_EVENTS];
    uint16_t head;
    uint16_t tail;
    uint16_t count;
} event_queue_t;

// Function prototypes
void event_init(void);
int event_poll(event_t *event);
void handle_event(event_t event);

#endif // EVENT_H