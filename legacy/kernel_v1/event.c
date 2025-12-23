#include "event.h"

// Event queue instance
#define MAX_EVENTS 64
static struct
{
    event_t events[MAX_EVENTS];
    uint16_t head;
    uint16_t tail;
    uint16_t count;
} event_queue;

// Initialize event system
void event_init(void)
{
    event_queue.head = 0;
    event_queue.tail = 0;
    event_queue.count = 0;
}

// Poll for events
int event_poll(event_t *event)
{
    if (event_queue.count == 0)
        return 0;

    // Get the next event
    *event = event_queue.events[event_queue.head];

    // Update queue
    event_queue.head = (event_queue.head + 1) % MAX_EVENTS;
    event_queue.count--;

    return 1;
}

// Handle events
void handle_event(event_t event)
{
    // Handle events based on type
    switch (event.type)
    {
    case EVENT_KEY_PRESS:
        // Handle key press
        break;
    case EVENT_KEY_RELEASE:
        // Handle key release
        break;
    case EVENT_MOUSE_MOVE:
        // Handle mouse move
        break;
    case EVENT_MOUSE_BUTTON:
        // Handle mouse button
        break;
    default:
        // Unknown event type
        break;
    }
}